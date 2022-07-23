#ifndef TESTMARKETGATEWAY_H
#define TESTMARKETGATEWAY_H

#include "MarketGateWay.hpp"


class TestMarketGateWay : public MarketGateWay
{
public:
    explicit TestMarketGateWay();
    virtual ~TestMarketGateWay();
public:
    virtual bool LoadAPIConfig();
    virtual void Run();
protected:
    MarketData::TFutureMarketData m_MarketData;
};

#endif // TESTMARKETGATEWAY_H