#include "XMarketCenter.h"

using namespace std;
extern Utils::Logger *gLogger;

XMarketCenter::XMarketCenter()
{
    m_PackClient = NULL;
    m_MarketGateWay = NULL;
    m_LastTick = -1;
}

XMarketCenter::~XMarketCenter()
{
    if (NULL != m_PackClient)
    {
        delete m_PackClient;
        m_PackClient = NULL;
        delete m_MarketGateWay;
        m_MarketGateWay = NULL;
    }
}

bool XMarketCenter::LoadConfig(const char *yml)
{
    bool ret = true;
    std::string errorBuffer;
    if(Utils::LoadMarketCenterConfig(yml, m_MarketCenterConfig, errorBuffer))
    {
        Utils::gLogger->Log->info("XMarketCenter::LoadConfig {} successed", yml);
        char buffer[512] = {0};
        sprintf(buffer, "MarketCenter::LoadConfig ExchangeID:%s ServerIP:%s Port:%d TotalTick:%d MarketChannelKey:0X%X "
                "RecvTimeOut:%d APIConfig:%s TickerListPath:%s ToMonitor:%d Future:%d",
                m_MarketCenterConfig.ExchangeID.c_str(), m_MarketCenterConfig.ServerIP.c_str(), 
                m_MarketCenterConfig.Port, m_MarketCenterConfig.TotalTick,
                m_MarketCenterConfig.MarketChannelKey, m_MarketCenterConfig.RecvTimeOut, 
                m_MarketCenterConfig.APIConfig.c_str(), m_MarketCenterConfig.TickerListPath.c_str(),
                m_MarketCenterConfig.ToMonitor, m_MarketCenterConfig.Future);
        Utils::gLogger->Log->info(buffer);
        std::vector<std::string> CPUSETVector;
        Utils::Split(m_MarketCenterConfig.CPUSET, ",", CPUSETVector);
        for(int i = 0; i < CPUSETVector.size(); i++)
        {
            m_CPUSETVector.push_back(atoi(CPUSETVector.at(i).c_str()));
        }
    }
    else
    {
        ret = false;
        Utils::gLogger->Log->error("XMarketCenter::LoadConfig {} failed, {}", yml, errorBuffer);
    }
    if(Utils::LoadTickerList(m_MarketCenterConfig.TickerListPath.c_str(), m_TickerPropertyVec, errorBuffer))
    {
        Utils::gLogger->Log->info("XMarketCenter::LoadTickerList {} successed", m_MarketCenterConfig.TickerListPath);
        for(auto it = m_TickerPropertyVec.begin(); it != m_TickerPropertyVec.end(); ++it)
        {
            if(m_MarketCenterConfig.ExchangeID == it->ExchangeID)
            {
                m_TickerExchangeMap[it->Ticker] = it->ExchangeID;
            }
            m_TickerIndexMap[it->Ticker] = it->Index;
        }
    }
    else
    {
        ret = false;
        Utils::gLogger->Log->error("XMarketCenter::LoadTickerList {} failed, {}", m_MarketCenterConfig.TickerListPath, errorBuffer);
    }
    return ret;
}

void XMarketCenter::SetCommand(const std::string& cmd)
{
    m_Command = cmd;
    Utils::gLogger->Log->info("XMarketCenter::SetCommand cmd:{}", m_Command);
}

void XMarketCenter::LoadMarketGateWay(const std::string& soPath)
{
    std::string errorString;
    m_MarketGateWay = XPluginEngine<MarketGateWay>::LoadPlugin(soPath, errorString);
    if(NULL == m_MarketGateWay)
    {
        Utils::gLogger->Log->error("XMarketCenter::LoadMarketGateWay {} failed, {}", soPath, errorString);
        sleep(1);
        exit(-1);
    }
    else
    {
        Utils::gLogger->Log->info("XMarketCenter::LoadMarketGateWay {} successed", soPath);
        m_MarketGateWay->SetLogger(Utils::Singleton<Utils::Logger>::GetInstance());
        m_MarketGateWay->SetMarketConfig(m_MarketCenterConfig);
    }
}

void XMarketCenter::Run()
{
    Utils::gLogger->Log->info("XMarketCenter::Run");
    // 信息采集客户端
    m_PackClient = new HPPackClient(m_MarketCenterConfig.ServerIP.c_str(), m_MarketCenterConfig.Port);
    m_PackClient->Start();
    sleep(1);
    m_PackClient->Login(m_MarketCenterConfig.ExchangeID.c_str());
    sleep(1);
    // MarketData Logger init
    m_MarketDataLogger = Utils::Singleton<Utils::MarketDataLogger>::GetInstance();
    m_MarketDataLogger->Init();

    // Update App Status
    InitAppStatus();

    // start thread to pull market data
    m_pPullThread = new std::thread(&XMarketCenter::PullMarketData, this);
    m_pHandleThread = new std::thread(&XMarketCenter::HandleMarketData, this);
    m_pPullThread->join();
    m_pHandleThread->join();
}

void XMarketCenter::PullMarketData()
{
    bool ret = Utils::ThreadBind(pthread_self(), m_CPUSETVector.at(0));
    if (NULL != m_MarketGateWay)
    {
        Utils::gLogger->Log->info("XMarketCenter::PullMarketData start thread to pull Market Data. CPU:{} CPUBind:{}", m_CPUSETVector.at(0), ret);
        m_MarketGateWay->Run();
    }
}

void XMarketCenter::HandleMarketData()
{
    bool ret = Utils::ThreadBind(pthread_self(), m_CPUSETVector.at(1));
    Utils::gLogger->Log->info("XMarketCenter::HandleMarketData start thread CPU:{} CPUBind:{}", m_CPUSETVector.at(1), ret);
    if(m_MarketCenterConfig.Future)
    {
        Utils::gLogger->Log->info("XMarketCenter::HandleMarketData Future Market Data");
        Utils::IPCMarketQueue<MarketData::TFutureMarketDataSet> MarketQueue(m_MarketCenterConfig.TotalTick, m_MarketCenterConfig.MarketChannelKey);
        // Init Market Queue
        {
            for (size_t i = 0; i < m_MarketCenterConfig.TotalTick; i++)
            {
                MarketData::TFutureMarketDataSet dataset;
                MarketQueue.Read(i, dataset);
                InitMarketData(i, dataset);
                MarketQueue.Write(i, dataset);
            }
            MarketQueue.ResetTick(0);
        }
        while (true)
        {
            // reload PackClient when timeout greater equals to 10s
            static unsigned long prevtimestamp = Utils::getTimeMs();
            unsigned long currenttimestamp = Utils::getTimeMs();
            if(currenttimestamp - prevtimestamp >=  10 * 1000)
            {
                m_PackClient->ReConnect();
                prevtimestamp = currenttimestamp;
            }
            int TickIndex = 0;
            int recvCount = 0;
            int TimeOut = 0;
            int SectionLastTick = 0;
            bool recvFuturesDone = false;
            std::string StartRecvTime;
            std::string StopRecvTime;
            std::string LastTicker;
            // receive data from queue to update last data
            while (true)
            {
                Message::PackMessage message;
                bool ret = m_MarketGateWay->m_MarketMessageQueue.pop(message);
                if (ret)
                {
                    auto it = m_TickerExchangeMap.find(message.FutureMarketData.Ticker);
                    if (it != m_TickerExchangeMap.end())
                    {
                        message.FutureMarketData.TotalTick = m_MarketCenterConfig.TotalTick;
                        strncpy(message.FutureMarketData.ExchangeID, it->second.c_str(), sizeof(message.FutureMarketData.ExchangeID));
                        Utils::CalculateTick(m_MarketCenterConfig, message.FutureMarketData);
                        MarketData::Check(message.FutureMarketData);
                    }
                    TickIndex = message.FutureMarketData.Tick;
                    SectionLastTick = message.FutureMarketData.SectionLastTick;
                    if (0 == recvCount)
                        StartRecvTime = message.FutureMarketData.RevDataLocalTime + 11;
                    LastTicker = message.FutureMarketData.Ticker;
                    // invalid data
                    if(1 != message.FutureMarketData.ErrorID)
                    {
                        m_LastFutureMarketDataMap[LastTicker] = message.FutureMarketData;
                        m_MarketDataSetVector.push_back(message.FutureMarketData);
                    }
                    recvCount++;
                    // 继续收取行情，直到收取所有Ticker行情
                    if (recvCount >= m_TickerExchangeMap.size())
                    {
                        recvFuturesDone = true;
                        StopRecvTime = Utils::getCurrentTimeUs() + 11;
                        break;
                    }
                    else
                    {
                        continue;
                    }
                }
                // 如果行情数据没有收取完成，超时处理
                if (!recvFuturesDone && !StartRecvTime.empty())
                {
                    StopRecvTime = Utils::getCurrentTimeUs() + 11;
                    TimeOut = Utils::TimeDiffUs(StartRecvTime, StopRecvTime);
                    if (TimeOut > m_MarketCenterConfig.RecvTimeOut)
                    {
                        recvFuturesDone = true;
                        Utils::gLogger->Log->debug("XMarketCenter::HandleMarketData Recv:{}, TimeOut:{}, begin:{}, end:{}, Last Tick:{}",
                                                       recvCount, TimeOut, StartRecvTime, StopRecvTime, TickIndex);
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
            // Update Future Market Data when All Ticker received 
            if(recvFuturesDone)
            {
                MarketData::TFutureMarketDataSet dataset;
                UpdateFutureMarketData(TickIndex, dataset);
                std::string TickUpdateTime = Utils::getCurrentTimeUs();
                dataset.Tick = TickIndex;
                memcpy(dataset.UpdateTime, TickUpdateTime.c_str(), sizeof(dataset.UpdateTime));
                // Write MarketData to Share Memory Queue
                MarketQueue.Write(TickIndex, dataset);
                // send Market Data to XServer
                UpdateLastMarketData();
                Utils::gLogger->Log->info("XMarketCenter::HandleMarketData Future Recv:{}, TimeOut:{}, Last Ticker:{}, begin:{}, end:{}, CurrentTime:{} Last Tick:{}",
                                        recvCount, TimeOut, LastTicker, StartRecvTime, StopRecvTime, Utils::getCurrentTimeUs() + 11, TickIndex);
                // Section结束重复推送Tick
                if(TickIndex == SectionLastTick && m_LastTick == SectionLastTick)
                    continue;
                // Write Market Data to disk
                m_MarketDataLogger->WriteMarketDataSet(dataset);
                m_LastTick = TickIndex;
            }
            // Update Spot Market Data
            while (true)
            {
                Message::PackMessage message;
                bool ret = m_PackClient->m_MarketMessageQueue.pop(message);
                if(ret)
                {
                    Utils::gLogger->Log->debug("XMarketCenter::HandleMarketData Receive Spot Ticker:{}", message.FutureMarketData.Ticker);
                    auto it = m_TickerIndexMap.find(message.FutureMarketData.Ticker);
                    if(it != m_TickerIndexMap.end())
                    {
                        m_LastFutureMarketDataMap[message.FutureMarketData.Ticker] = message.FutureMarketData;
                    }
                }
                else
                {
                    break;
                }
            }
        }
    }
    else
    {
        Utils::gLogger->Log->info("XMarketCenter::HandleMarketData Spot Market Data");
        // 现货行情数据处理
        while(true)
        {
            // reload PackClient when timeout greater equals to 10s
            static unsigned long prevtimestamp = Utils::getTimeMs();
            unsigned long currenttimestamp = Utils::getTimeMs();
            if(currenttimestamp - prevtimestamp >=  10 * 1000)
            {
                m_PackClient->ReConnect();
                prevtimestamp = currenttimestamp;
            }
            Message::PackMessage message;
            bool ret = m_MarketGateWay->m_MarketMessageQueue.pop(message);
            if(ret)
            {
                // Forward to Monitor
                if(m_MarketCenterConfig.ToMonitor)
                {
                    m_PackClient->SendData(reinterpret_cast<const unsigned char *>(&message), sizeof(message));
                }
                Utils::gLogger->Log->debug("XMarketCenter::HandleMarketData Ticker:{} UpdateTime:{}",
                                         message.SpotMarketData.Ticker, message.SpotMarketData.UpdateTime);
            }
            m_PackClient->m_MarketMessageQueue.pop(message);
        }
    }
}

void XMarketCenter::UpdateFutureMarketData(int tick, MarketData::TFutureMarketDataSet& dataset)
{
    for(auto it = m_LastFutureMarketDataMap.begin(); it != m_LastFutureMarketDataMap.end(); it++)
    {
        it->second.LastTick = tick;
        std::string ticker = it->first;
        int index = m_TickerIndexMap[ticker];
        dataset.MarketData[index] = it->second;
    }
}

void XMarketCenter::InitMarketData(int tick, MarketData::TFutureMarketDataSet &dataset)
{
    dataset.Tick = tick;
    for(auto it = m_TickerIndexMap.begin(); it != m_TickerIndexMap.end(); it++)
    {
        int index = it->second;
        strncpy(dataset.MarketData[index].Ticker, it->first.c_str(), sizeof(dataset.MarketData[index].Ticker));
    }
}

void XMarketCenter::UpdateLastMarketData()
{
    for(int i = 0; i < m_MarketDataSetVector.size(); i++)
    {
        Message::PackMessage message;
        message.MessageType = Message::EMessageType::EFutureMarketData;
        memcpy(&message.FutureMarketData, &m_MarketDataSetVector.at(i), sizeof(message.FutureMarketData));
        if(m_MarketDataSetVector.size() == i + 1)
        {
            message.FutureMarketData.IsLast = true;
        }
        else
        {
            message.FutureMarketData.IsLast = false;
        }
        // Forward to Monitor
        if(m_MarketCenterConfig.ToMonitor)
        {
            m_PackClient->SendData(reinterpret_cast<const unsigned char *>(&message), sizeof(message));
        }
    }
    m_MarketDataSetVector.clear();
}

void XMarketCenter::InitAppStatus()
{
    Message::PackMessage message;
    message.MessageType = Message::EMessageType::EAppStatus;
    UpdateAppStatus(m_Command, message.AppStatus);
    m_PackClient->SendData((const unsigned char*)&message, sizeof(message));
}

void XMarketCenter::UpdateAppStatus(const std::string& cmd, Message::TAppStatus& AppStatus)
{
    std::vector<std::string> ItemVec;
    Utils::Split(cmd, " ", ItemVec);
    std::string Account;
    for(int i = 0; i < ItemVec.size(); i++)
    {
        if(Utils::equalWith(ItemVec.at(i), "-a"))
        {
            Account = ItemVec.at(i + 1);
            break;
        }
    }
    strncpy(AppStatus.Account, Account.c_str(), sizeof(AppStatus.Account));

    std::vector<std::string> Vec;
    Utils::Split(ItemVec.at(0), "/", Vec);
    std::string AppName = Vec.at(Vec.size() - 1);
    strncpy(AppStatus.AppName, AppName.c_str(), sizeof(AppStatus.AppName));
    AppStatus.PID = getpid();
    strncpy(AppStatus.Status, "Start", sizeof(AppStatus.Status));

    char command[512] = {0};
    std::string AppLogPath;
    char* p = getenv("APP_LOG_PATH");
    if(p == NULL)
    {
        AppLogPath = "./log/";
    }
    else
    {
        AppLogPath = p;
    }
    sprintf(command, "nohup %s > %s/%s_%s_run.log 2>&1 &", cmd.c_str(), AppLogPath.c_str(), 
            AppName.c_str(), AppStatus.Account);
    strncpy(AppStatus.StartScript, command, sizeof(AppStatus.StartScript));
    std::string SoCommitID;
    std::string SoUtilsCommitID;
    m_MarketGateWay->GetCommitID(SoCommitID, SoUtilsCommitID);
    std::string CommitID = std::string(APP_COMMITID) + ":" + SoCommitID;
    strncpy(AppStatus.CommitID, CommitID.c_str(), sizeof(AppStatus.CommitID));
    std::string UtilsCommitID = std::string(UTILS_COMMITID) + ":" + SoUtilsCommitID;
    strncpy(AppStatus.UtilsCommitID, UtilsCommitID.c_str(), sizeof(AppStatus.UtilsCommitID));
    std::string APIVersion;
    m_MarketGateWay->GetAPIVersion(APIVersion);
    strncpy(AppStatus.APIVersion, APIVersion.c_str(), sizeof(AppStatus.APIVersion));
    strncpy(AppStatus.StartTime, Utils::getCurrentTimeUs(), sizeof(AppStatus.StartTime));
    strncpy(AppStatus.LastStartTime, Utils::getCurrentTimeUs(), sizeof(AppStatus.LastStartTime));
    strncpy(AppStatus.UpdateTime, Utils::getCurrentTimeUs(), sizeof(AppStatus.UpdateTime));
}
