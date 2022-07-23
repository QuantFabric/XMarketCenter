#include "TestMarketGateWay.h"
#include "XPluginEngine.hpp"

CreateObjectFunc(TestMarketGateWay);

TestMarketGateWay::TestMarketGateWay() : MarketGateWay()
{

}

TestMarketGateWay::~TestMarketGateWay()
{

}

bool TestMarketGateWay::LoadAPIConfig()
{
    m_Logger->Log->info("TestMarketGateWay::LoadAPIConfig");
    // Load API Config

    return true;
}

void TestMarketGateWay::Run()
{
    m_Logger->Log->info("TestMarketGateWay::Run Start Test Market GateWay");
    while(true)
    {
        static int Tick = 0;
        memset(&m_MarketData, 0, sizeof(m_MarketData));
        m_MarketData.Tick = Tick++;
        strncpy(m_MarketData.Ticker, "IF2209", sizeof(m_MarketData.Ticker));
        memcpy(m_MarketData.RevDataLocalTime, Utils::getCurrentTimeUs(), sizeof(m_MarketData.RevDataLocalTime));
        strncpy(m_MarketData.ExchangeID, "CFFEX", sizeof(m_MarketData.ExchangeID));
        // 写入行情数据到行情队列
        Message::PackMessage message;
        message.MessageType = Message::EMessageType::EFutureMarketData;
        memcpy(&message.FutureMarketData, &m_MarketData, sizeof(message.FutureMarketData));
        bool ret = m_MarketMessageQueue.push(message);
        m_Logger->Log->info("TestMarketGateWay::Run Pull Market Data, UpdateTime:{}", Utils::getCurrentTimeUs());

        usleep(500*1000);
    }
}