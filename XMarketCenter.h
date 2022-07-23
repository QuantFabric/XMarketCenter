#ifndef MARKETCENTER_H
#define MARKETCENTER_H

#include <unordered_map>
#include <algorithm>
#include "Singleton.hpp"
#include "YMLConfig.hpp"
#include "Logger.h"
#include "PackMessage.hpp"
#include "XPluginEngine.hpp"
#include "MarketData.hpp"
#include "MarketDataLogger.hpp"
#include "IPCMarketQueue.hpp"
#include "MarketAPI/MarketGateWay.hpp"
#include "HPPackClient.h"

class XMarketCenter
{
public:
    explicit XMarketCenter();
    virtual ~XMarketCenter();
    bool LoadConfig(const char *yml);
    void SetCommand(const std::string& cmd);
    void LoadMarketGateWay(const std::string& soPath);
    void Run();
protected:
    // pull Market Data
    void PullMarketData();
    void HandleMarketData();
    // update Market Data from queue
    void UpdateFutureMarketData(int tick, MarketData::TFutureMarketDataSet &dataset);
    void InitMarketData(int tick, MarketData::TFutureMarketDataSet &dataset);
    void UpdateLastMarketData();
private:
    Utils::MarketCenterConfig m_MarketCenterConfig;
    std::vector<Utils::TickerProperty> m_TickerPropertyVec;
    std::unordered_map<std::string, std::string> m_TickerExchangeMap;
    std::unordered_map<std::string, int> m_TickerIndexMap;
    std::string m_Command;
    HPPackClient *m_PackClient;
    MarketGateWay* m_MarketGateWay;
    std::thread* m_pHandleThread;
    std::thread* m_pPullThread;
    Utils::MarketDataLogger *m_MarketDataLogger;
    std::unordered_map<std::string, MarketData::TFutureMarketData> m_LastFutureMarketDataMap;
    std::vector<MarketData::TFutureMarketData> m_MarketDataSetVector;
    int m_LastTick;
};

#endif // MARKETCENTER_H