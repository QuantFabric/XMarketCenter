#include "XMarketCenter.h"

using namespace std;
extern Utils::Logger *gLogger;

XMarketCenter::XMarketCenter()
{
    m_PackClient = NULL;
    m_PubServer = NULL;
    m_MarketGateWay = NULL;
}

XMarketCenter::~XMarketCenter()
{
    if (NULL != m_PackClient)
    {
        delete m_PackClient;
        m_PackClient = NULL;
        delete m_PubServer;
        m_PubServer = NULL;
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
        Utils::gLogger->Log->info("MarketCenter::LoadConfig ExchangeID:{} ServerIP:{} Port:{} TotalTick:{} MarketServer:{} "
                "RecvTimeOut:{} APIConfig:{} TickerListPath:{} ToMonitor:{} Future:{}",
                m_MarketCenterConfig.ExchangeID, m_MarketCenterConfig.ServerIP, 
                m_MarketCenterConfig.Port, m_MarketCenterConfig.TotalTick,
                m_MarketCenterConfig.MarketServer, m_MarketCenterConfig.RecvTimeOut, 
                m_MarketCenterConfig.APIConfig, m_MarketCenterConfig.TickerListPath,
                m_MarketCenterConfig.ToMonitor, m_MarketCenterConfig.Future);
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
    // 信息采集客户端
    m_PackClient = new HPPackClient(m_MarketCenterConfig.ServerIP.c_str(), m_MarketCenterConfig.Port);
    m_PackClient->Start();
    sleep(1);
    m_PackClient->Login(m_MarketCenterConfig.ExchangeID.c_str());
    sleep(1);

    // Update App Status
    InitAppStatus();
    // 启动行情发布服务
    Utils::gLogger->Log->info("XMarketCenter::Run Start PubServer");
    m_PubServer = new PubServer();
    m_PubServer->Start(m_MarketCenterConfig.MarketServer);

    m_pPullThread = new std::thread(&XMarketCenter::PullMarketData, this);
    m_pHandleThread = new std::thread(&XMarketCenter::HandleMarketData, this);
    
    m_pPullThread->join();
    m_pHandleThread->join();
}

void XMarketCenter::PullMarketData()
{
    if(NULL != m_MarketGateWay)
    {
        Utils::gLogger->Log->info("XMarketCenter::PullMarketData start thread to pull Market Data. ");
        m_MarketGateWay->Run();
    }
}

void XMarketCenter::HandleMarketData()
{
    Utils::gLogger->Log->info("XMarketCenter::HandleMarketData start thread");
    SHMIPC::ChannelMsg<Message::PackMessage> msg;
    if(m_MarketCenterConfig.BussinessType == Message::EBusinessType::EFUTURE)
    {
        Utils::gLogger->Log->info("XMarketCenter::HandleMarketData Future Market Data");
        while (true)
        {
            // receive data from queue to update last data
            while (true)
            {
                bool ret = m_MarketGateWay->m_MarketMessageQueue.Pop(msg.Data);
                if(ret)
                {
                    MarketData::Check(msg.Data.FutureMarketData);
                    Utils::gLogger->Log->debug("XMarketCenter::HandleMarketData Receive Future Ticker:{}", msg.Data.FutureMarketData.Ticker);
                    if(!m_PubServer->m_SendQueue.Push(msg))
                    {
                        Utils::gLogger->Log->error("XMarketCenter::HandleMarketData Send Future Data failed, Ticker:{} PubServer m_SendQueue full", 
                                                msg.Data.FutureMarketData.Ticker);
                    }
                    // Forward to Monitor
                    if(m_MarketCenterConfig.ToMonitor)
                    {
                        m_PackClient->SendData(reinterpret_cast<const unsigned char *>(&msg.Data), sizeof(msg.Data));
                    }
                }
                else
                {
                    // reload PackClient when timeout greater equals to 10s
                    static unsigned long prevtimestamp = Utils::getTimeMs();
                    unsigned long currenttimestamp = Utils::getTimeMs();
                    if(currenttimestamp - prevtimestamp >=  10 * 1000)
                    {
                        m_PackClient->ReConnect();
                        prevtimestamp = currenttimestamp;
                    }
                    break;
                }
            }
            // Update Spot Market Data
            while (true)
            {
                bool ret = m_PackClient->m_MarketMessageQueue.Pop(msg.Data);
                if(ret)
                {
                    Utils::gLogger->Log->debug("XMarketCenter::HandleMarketData Receive Spot Ticker:{}", msg.Data.FutureMarketData.Ticker);
                    if(!m_PubServer->m_SendQueue.Push(msg))
                    {
                        Utils::gLogger->Log->error("XMarketCenter::HandleMarketData Send Spot Data failed, Ticker:{} PubServer m_SendQueue full", 
                                                msg.Data.FutureMarketData.Ticker);
                    }
                }
                else
                {
                    break;
                }
            }
        }
    }
    else if(m_MarketCenterConfig.BussinessType == Message::EBusinessType::ESPOT)
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
            bool ret = m_MarketGateWay->m_MarketMessageQueue.Pop(msg.Data);
            if(ret)
            {
                // Forward to Monitor
                if(m_MarketCenterConfig.ToMonitor)
                {
                    m_PackClient->SendData(reinterpret_cast<const unsigned char *>(&msg.Data), sizeof(msg.Data));
                }
                Utils::gLogger->Log->debug("XMarketCenter::HandleMarketData Ticker:{} UpdateTime:{}",
                                         msg.Data.SpotMarketData.Ticker, msg.Data.SpotMarketData.UpdateTime);
            }
            m_PackClient->m_MarketMessageQueue.Pop(msg.Data);
        }
    }
    else if(m_MarketCenterConfig.BussinessType == Message::EBusinessType::ESTOCK)
    {
        while(true)
        {
            // receive data from queue to update last data
            while (true)
            {
                bool ret = m_MarketGateWay->m_MarketMessageQueue.Pop(msg.Data);
                if(ret)
                {
                    Utils::gLogger->Log->debug("XMarketCenter::HandleMarketData Receive Stock Ticker:{}", msg.Data.StockMarketData.Ticker);
                    if(!m_PubServer->m_SendQueue.Push(msg))
                    {
                        Utils::gLogger->Log->error("XMarketCenter::HandleMarketData Send Stock Data failed, Ticker:{} PubServer m_SendQueue full", 
                                                msg.Data.StockMarketData.Ticker);
                    }
                    // Forward to Monitor
                    if(m_MarketCenterConfig.ToMonitor)
                    {
                        m_PackClient->SendData(reinterpret_cast<const unsigned char *>(&msg.Data), sizeof(msg.Data));
                    }
                }
                else
                {
                    // reload PackClient when timeout greater equals to 10s
                    static unsigned long prevtimestamp = Utils::getTimeMs();
                    unsigned long currenttimestamp = Utils::getTimeMs();
                    if(currenttimestamp - prevtimestamp >=  10 * 1000)
                    {
                        m_PackClient->ReConnect();
                        prevtimestamp = currenttimestamp;
                    }
                    break;
                }
            }
        }
    }
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
