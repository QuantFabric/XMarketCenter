#include "XMarketCenter.h"
#include <unistd.h>

using namespace std;
extern Utils::Logger *gLogger;

XMarketCenter::XMarketCenter()
{
    m_MarketGateWay = NULL;
    m_CTPMarketDataLogger = NULL;
}

XMarketCenter::~XMarketCenter()
{
    if (NULL != m_MarketGateWay)
    {
        delete m_MarketGateWay;
        m_MarketGateWay = NULL;
        delete m_CTPMarketDataLogger;
        m_CTPMarketDataLogger = NULL;
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
        sprintf(buffer, "MarketCenter::LoadConfig ExchangeID:%s ServerIP:%s Port:%d TotalTick:%d MarketServer:%s "
                "RecvTimeOut:%d APIConfig:%s TickerListPath:%s ToMonitor:%d Future:%d",
                m_MarketCenterConfig.ExchangeID.c_str(), m_MarketCenterConfig.ServerIP.c_str(), 
                m_MarketCenterConfig.Port, m_MarketCenterConfig.TotalTick,
                m_MarketCenterConfig.MarketServer.c_str(), m_MarketCenterConfig.RecvTimeOut, 
                m_MarketCenterConfig.APIConfig.c_str(), m_MarketCenterConfig.TickerListPath.c_str(),
                m_MarketCenterConfig.ToMonitor, m_MarketCenterConfig.Future);
        Utils::gLogger->Log->info(buffer);
    }
    else
    {
        ret = false;
        Utils::gLogger->Log->error("XMarketCenter::LoadConfig {} failed, {}", yml, errorBuffer);
    }
    if(Utils::LoadTickerList(m_MarketCenterConfig.TickerListPath.c_str(), m_TickerPropertyVec, errorBuffer))
    {
        Utils::gLogger->Log->info("XMarketCenter::LoadTickerList {} successed", m_MarketCenterConfig.TickerListPath);
    }
    else
    {
        ret = false;
        Utils::gLogger->Log->error("XMarketCenter::LoadTickerList {} failed, {}", m_MarketCenterConfig.TickerListPath, errorBuffer);
    }
    return ret;
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
    // MarketData Logger init
    m_CTPMarketDataLogger = Utils::Singleton<CTPMarketDataLogger>::GetInstance();

    // start thread to pull market data
    m_pPullThread = new std::thread(&XMarketCenter::PullMarketData, this);
    m_pHandleThread = new std::thread(&XMarketCenter::HandleMarketData, this);
    m_pPullThread->join();
    m_pHandleThread->join();
}

void XMarketCenter::PullMarketData()
{
    if (NULL != m_MarketGateWay)
    {
        Utils::gLogger->Log->info("XMarketCenter::PullMarketData start thread to pull Market Data.");
        m_MarketGateWay->Run();
    }
}

void XMarketCenter::HandleMarketData()
{
    Utils::gLogger->Log->info("XMarketCenter::HandleMarketData start thread to handle Market Data.");
    while(true)
    {
        Message::PackMessage message;
        bool ret = m_MarketGateWay->m_MarketMessageQueue.Pop(message);
        if(ret)
        {
            Check(message.FutureMarketData);
            m_CTPMarketDataLogger->WriteFutureMarketData(message.FutureMarketData);
            Utils::gLogger->Log->debug("XMarketCenter::HandleMarketData Ticker:{} UpdateTime:{}",
                                        message.FutureMarketData.Ticker, message.FutureMarketData.UpdateTime);
        }
        else
        {
            usleep(5);
        }
    }
}

