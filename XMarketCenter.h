#ifndef MARKETCENTER_H
#define MARKETCENTER_H

#include <unordered_map>
#include <algorithm>
#include "YMLConfig.hpp"
#include "Logger.h"
#include "XPluginEngine.hpp"
#include "CTPMarketDataLogger.hpp"
#include "RingBuffer.hpp"
#include "MarketAPI/MarketGateWay.hpp"

class XMarketCenter
{
public:
    explicit XMarketCenter();
    virtual ~XMarketCenter();
    bool LoadConfig(const char *yml);
    void LoadMarketGateWay(const std::string& soPath);
    void Run();
protected:
    void PullMarketData();
    void HandleMarketData();
private:
    Utils::MarketCenterConfig m_MarketCenterConfig;
    std::vector<Utils::TickerProperty> m_TickerPropertyVec;
    std::unordered_map<std::string, std::string> m_TickerExchangeMap;
    MarketGateWay* m_MarketGateWay;
    std::thread* m_pHandleThread;
    std::thread* m_pPullThread;
    CTPMarketDataLogger* m_CTPMarketDataLogger;
};

#endif // MARKETCENTER_H