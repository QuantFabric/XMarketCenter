#ifndef MARKETGATEWAY_H
#define MARKETGATEWAY_H

#include "LockFreeQueue.hpp"
#include "PackMessage.hpp"
#include "YMLConfig.hpp"
#include "Logger.h"

class MarketGateWay
{
public:
    virtual bool LoadAPIConfig() = 0;
    virtual void Run() = 0;
    virtual void GetCommitID(std::string& CommitID, std::string& UtilsCommitID) = 0;
    virtual void GetAPIVersion(std::string& APIVersion) = 0;
public:
    explicit MarketGateWay(): m_MarketMessageQueue(1 << 14)
    {
        m_Logger = NULL;
    }

    void SetLogger(Utils::Logger* logger)
    {
        if(NULL == logger)
        {
            printf("Logger is NULL\n");
            exit(-1);
        }
        m_Logger = logger;
    }

    void SetMarketConfig(const Utils::MarketCenterConfig& config)
    {
        m_MarketCenterConfig = config;
        LoadAPIConfig();
        std::string errorString;
        bool ok = Utils::LoadTickerList(m_MarketCenterConfig.TickerListPath.c_str(), m_TickerPropertyList, errorString);
        if(ok)
        {
            m_Logger->Log->info("MarketGateWay::LoadTickerList {} successed", m_MarketCenterConfig.TickerListPath);
            for(auto it = m_TickerPropertyList.begin(); it != m_TickerPropertyList.end(); it++)
            {
                m_TickerExchangeMap[it->Ticker] = it->ExchangeID;
                m_TickerPropertyMap[it->Ticker] = *it;
                m_Logger->Log->info("MarketGateWay::LoadTickerList Ticker:{}", it->Ticker);
            }
        }
        else
        {
            m_Logger->Log->warn("MarketGateWay::LoadTickerList {} failed, {}", m_MarketCenterConfig.TickerListPath, errorString);
        }
    }
protected:
    void PrintMarketData(const MarketData::TFutureMarketData &data)
    {
        m_Logger->Log->debug("================PrintMarketData=================");
        m_Logger->Log->debug("Tick:{}", data.Tick);
        m_Logger->Log->debug("Ticker:{}", data.Ticker);
        m_Logger->Log->debug("ExchangeID:{}", data.ExchangeID);
        m_Logger->Log->debug("UpdateTime:{}", data.UpdateTime);
        m_Logger->Log->debug("MillSec:{}", data.MillSec);
        m_Logger->Log->debug("LastPrice:{}", data.LastPrice);
        m_Logger->Log->debug("Turnover:{}", data.Turnover);
        m_Logger->Log->debug("OpenPrice:{}", data.OpenPrice);
        m_Logger->Log->debug("HighestPrice:{}", data.HighestPrice);
        m_Logger->Log->debug("LowestPrice:{}", data.LowestPrice);
        m_Logger->Log->debug("OpenInterest:{}", data.OpenInterest);
        m_Logger->Log->debug("PreSettlementPrice:{}", data.PreSettlementPrice);
        m_Logger->Log->debug("PreClosePrice:{}", data.PreClosePrice);
        m_Logger->Log->debug("AskPrice5:{}", data.AskPrice5);
        m_Logger->Log->debug("AskPrice4:{}", data.AskPrice4);
        m_Logger->Log->debug("AskPrice3:{}", data.AskPrice3);
        m_Logger->Log->debug("AskPrice2:{}", data.AskPrice2);
        m_Logger->Log->debug("AskPrice1:{}", data.AskPrice1);
        m_Logger->Log->debug("BidPrice1:{}", data.BidPrice1);
        m_Logger->Log->debug("BidPrice2:{}", data.BidPrice2);
        m_Logger->Log->debug("BidPrice3:{}", data.BidPrice3);
        m_Logger->Log->debug("BidPrice4:{}", data.BidPrice4);
        m_Logger->Log->debug("BidPrice5:{}", data.BidPrice5);
        m_Logger->Log->debug("AskVolume5:{}", data.AskVolume5);
        m_Logger->Log->debug("AskVolume4:{}", data.AskVolume4);
        m_Logger->Log->debug("AskVolume3:{}", data.AskVolume3);
        m_Logger->Log->debug("AskVolume2:{}", data.AskVolume2);
        m_Logger->Log->debug("AskVolume1:{}", data.AskVolume1);
        m_Logger->Log->debug("BidVolume1:{}", data.BidVolume1);
        m_Logger->Log->debug("BidVolume2:{}", data.BidVolume2);
        m_Logger->Log->debug("BidVolume3:{}", data.BidVolume3);
        m_Logger->Log->debug("BidVolume4:{}", data.BidVolume4);
        m_Logger->Log->debug("BidVolume5:{}", data.BidVolume5);
    }
public:
    Utils::LockFreeQueue<Message::PackMessage> m_MarketMessageQueue;
protected:
    Utils::MarketCenterConfig m_MarketCenterConfig;
    Utils::Logger* m_Logger;
    std::vector<Utils::TickerProperty> m_TickerPropertyList;
    std::unordered_map<std::string, std::string> m_TickerExchangeMap;
    std::unordered_map<std::string, Utils::TickerProperty> m_TickerPropertyMap;
};

#endif // MARKETGATEWAY_H