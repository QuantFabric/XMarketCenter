#include "XueQiuMarketGateWay.h"
#include "XPluginEngine.hpp"

CreateObjectFunc(XueQiuMarketGateWay);

XueQiuMarketGateWay::XueQiuMarketGateWay() : CrawlerMarketGateWay()
{
    
}

XueQiuMarketGateWay::~XueQiuMarketGateWay()
{

}

bool XueQiuMarketGateWay::LoadAPIConfig()
{
    bool ret = true;
    std::string errorBuffer;
    if(Utils::LoadCrawlerMarkeSourceConfig(m_MarketCenterConfig.APIConfig.c_str(), m_CrawlerMarketSourceConfig, errorBuffer))
    {
        m_Logger->Log->info("XueQiuMarketGateWay::LoadCrawlerMarkeSourceConfig {} successed", m_MarketCenterConfig.APIConfig);
        m_Script = m_CrawlerMarketSourceConfig.Script;
        m_TimeOut = m_CrawlerMarketSourceConfig.TimeOut;
        m_Source = m_CrawlerMarketSourceConfig.Source;
        m_Logger->Log->info("XueQiuMarketGateWay::LoadCrawlerMarkeSourceConfig Source:{} TimeOut:{} Script:{}", 
                            m_Source, m_TimeOut, m_Script);
    }
    else
    {
        ret = false;
        m_Logger->Log->error("XueQiuMarketGateWay::LoadCrawlerMarkeSourceConfig {} failed, {}", m_MarketCenterConfig.APIConfig, errorBuffer);
    }
    return ret;
}

void XueQiuMarketGateWay::GetCommitID(std::string& CommitID, std::string& UtilsCommitID)
{
    CommitID = SO_COMMITID;
    UtilsCommitID = SO_UTILS_COMMITID;
}

void XueQiuMarketGateWay::GetAPIVersion(std::string& APIVersion)
{
    APIVersion = API_VERSION;
}

bool XueQiuMarketGateWay::ParseMarketData(const std::vector<std::string> &market, MarketData::TFutureMarketData &data)
{
    bool ret = true;
    memset(&data, 0, sizeof(data));
    // Ticker,OpenPrice,LastPrice,PreClosePrice,HighestPrice,LowestPrice,UpdateTime
    if(market.size() == 7)
    {
        memset(&data, 0, sizeof(data));
        strncpy(data.Ticker, market.at(0).c_str(), sizeof(data.Ticker));
        data.OpenPrice = atof(market.at(1).c_str());
        data.LastPrice = atof(market.at(2).c_str());
        data.PreClosePrice = atof(market.at(3).c_str());
        data.HighestPrice = atof(market.at(4).c_str());
        data.LowestPrice = atof(market.at(5).c_str());
        strncpy(data.UpdateTime, market.at(6).c_str(), sizeof(data.UpdateTime));
        strncpy(data.RevDataLocalTime, Utils::getCurrentTimeUs(), sizeof(data.RevDataLocalTime));
    }
    else
    {
        std::string line;
        for(int i = 0; i < market.size(); i++)
        {
            line += (market.at(i) + " ");
        }
        m_Logger->Log->error("XueQiuMarketGateWay::ParseMarketData fileds:{} line:{}", market.size(), line);
        ret = false;
    }
    return ret;
}