#include "CTPMarketGateWay.h"
#include "XPluginEngine.hpp"

CreateObjectFunc(CTPMarketGateWay);

CTPMarketGateWay::CTPMarketGateWay() : MarketGateWay()
{

}

bool CTPMarketGateWay::LoadAPIConfig()
{
    bool ret = true;
    std::string errorBuffer;
    if(Utils::LoadCTPMarkeSourceConfig(m_MarketCenterConfig.APIConfig.c_str(), m_CTPMarketSourceConfig, errorBuffer))
    {
        m_Logger->Log->info("CTPMarketGateWay::LoadCTPMarkeSourceConfig {} successed", m_MarketCenterConfig.APIConfig);
        m_Logger->Log->info("CTPMarketGateWay FrontAddr:{} BrokerID:{} Account:{} TickerListPath:{}", 
                            m_CTPMarketSourceConfig.FrontAddr, m_CTPMarketSourceConfig.BrokerID,
                            m_CTPMarketSourceConfig.Account, m_CTPMarketSourceConfig.TickerListPath);
    }
    else
    {
        ret = false;
        m_Logger->Log->error("CTPMarketGateWay::LoadCTPMarkeSourceConfig {} failed, {}", m_MarketCenterConfig.APIConfig, errorBuffer);
    }
    return ret;
}

void CTPMarketGateWay::Run()
{
    // 创建行情API实例
    m_pMdUserApi = CThostFtdcMdApi::CreateFtdcMdApi();
    // 注册事件类
    m_pMdUserApi->RegisterSpi(this);
    // 设置行情前置地址
    m_pMdUserApi->RegisterFront(const_cast<char *>(m_CTPMarketSourceConfig.FrontAddr.c_str()));
    m_pMdUserApi->Init();
    m_Logger->Log->info("CTPMarketGateWay::Start FrontAddress:{}", m_CTPMarketSourceConfig.FrontAddr);
    // 等到线程退出
    m_pMdUserApi->Join();
}

void CTPMarketGateWay::GetCommitID(std::string& CommitID, std::string& UtilsCommitID)
{
    CommitID = SO_COMMITID;
    UtilsCommitID = SO_UTILS_COMMITID;
}

void CTPMarketGateWay::GetAPIVersion(std::string& APIVersion)
{
    APIVersion = API_VERSION;
}

void CTPMarketGateWay::OnFrontConnected()
{
    m_Logger->Log->info("CTPMarketGateWay::OnFrontConnected 建立网络连接成功");
    CThostFtdcReqUserLoginField loginReq;
    memset(&loginReq, 0, sizeof(loginReq));
    strcpy(loginReq.BrokerID, m_CTPMarketSourceConfig.BrokerID.c_str());
    strcpy(loginReq.UserID, m_CTPMarketSourceConfig.Account.c_str());
    strcpy(loginReq.Password, m_CTPMarketSourceConfig.Password.c_str());
    m_Logger->Log->info("CTPMarketGateWay::OnFrontConnected FrontAddr:{} BrokerID:{} UserID:{}", 
                        m_CTPMarketSourceConfig.FrontAddr, m_CTPMarketSourceConfig.BrokerID, m_CTPMarketSourceConfig.Account);
    static int requestID = 0;
    int rt = m_pMdUserApi->ReqUserLogin(&loginReq, requestID++);
    if (!rt)
        m_Logger->Log->info("CTPMarketGateWay::ReqUserLogin 发送登录请求成功");
    else
        m_Logger->Log->warn("CTPMarketGateWay::ReqUserLogin 发送登录请求失败");
}

void CTPMarketGateWay::OnFrontDisconnected(int nReason)
{
    char buffer[32] = {0};
    sprintf(buffer, "0X%X", nReason);
    m_Logger->Log->warn("CTPMarketGateWay::OnFrontDisconnected 网络连接断开, 错误码:{}", buffer);
}

void CTPMarketGateWay::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
    if (bIsLast && !bResult)
    {
        m_Logger->Log->info("CTPMarketGateWay::OnRspUserLogin 账户登录成功");
        m_Logger->Log->info("CTPMarketGateWay::OnRspUserLogin 交易日:{}", pRspUserLogin->TradingDay);
        m_Logger->Log->info("CTPMarketGateWay::OnRspUserLogin 登录时间:{}", pRspUserLogin->LoginTime);
        m_Logger->Log->info("CTPMarketGateWay::OnRspUserLogin 经纪商:{}", pRspUserLogin->BrokerID);
        m_Logger->Log->info("CTPMarketGateWay::OnRspUserLogin 账户名:{}", pRspUserLogin->UserID);
        m_Logger->Log->info("CTPMarketGateWay::OnRspUserLogin SystemName:{}", pRspUserLogin->SystemName);
        m_Logger->Log->info("CTPMarketGateWay::OnRspUserLogin ApiVersion:{}", m_pMdUserApi->GetApiVersion());

        // 读取合约配置
        int total_count = m_TickerPropertyList.size();
        int batch_size = 50;
        int n = total_count / batch_size;
        int mod = total_count % batch_size;
        if(n == 0)
        {
            batch_size = mod;
        }
        int i = 0;
        for(; i < n; i++)
        {
            char* instruments[batch_size];
            for(int j = 0; j < batch_size; j++)
            {
                instruments[j] = new char[32];
                memset(instruments[j], 0, 32);
                strncpy(instruments[j], m_TickerPropertyList[i * batch_size + j].Ticker.c_str(), 32);
            }
            // 开始订阅行情
            int rt = m_pMdUserApi->SubscribeMarketData(instruments, batch_size);
            if (!rt)
                m_Logger->Log->info("CTPMarketGateWay::SubscribeMarketData 第{}批次{}个合约发送订阅行情请求成功", i, batch_size);
            else
                m_Logger->Log->warn("CTPMarketGateWay::SubscribeMarketData 第{}批次{}个合约发送订阅行情请求失败", i, batch_size);

            for (size_t j = 0; j < batch_size; j++)
            {
                delete[] instruments[j];
                instruments[j] = NULL;
            }
        }
        if(mod > 0)
        {
            batch_size = mod;
            char* instruments[batch_size];
            for(int j = 0; j < batch_size; j++)
            {
                instruments[j] = new char[32];
                memset(instruments[j], 0, 32);
                strncpy(instruments[j], m_TickerPropertyList[i * batch_size + j].Ticker.c_str(), 32);
            }
            // 开始订阅行情
            int rt = m_pMdUserApi->SubscribeMarketData(instruments, batch_size);
            if (!rt)
                m_Logger->Log->info("CTPMarketGateWay::SubscribeMarketData 第{}批次{}个合约发送订阅行情请求成功", i, batch_size);
            else
                m_Logger->Log->warn("CTPMarketGateWay::SubscribeMarketData 第{}批次{}个合约发送订阅行情请求失败", i, batch_size);

            for (size_t j = 0; j < batch_size; j++)
            {
                delete[] instruments[j];
                instruments[j] = NULL;
            }
        }
    }
    else
    {
        m_Logger->Log->warn("CTPMarketGateWay::OnRspUserLogin 返回错误，ErrorID={}, {}", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    }
}

void CTPMarketGateWay::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
    if (!bResult)
    {
        m_Logger->Log->info("CTPMarketGateWay::OnRspUserLogout 账户登出成功");
        m_Logger->Log->info("CTPMarketGateWay::OnRspUserLogout 经纪商:{}", pUserLogout->BrokerID);
        m_Logger->Log->info("CTPMarketGateWay::OnRspUserLogout 账户:{}", pUserLogout->UserID);
    }
    else
    {
        m_Logger->Log->warn("CTPMarketGateWay::OnRspUserLogout 返回错误，ErrorID={}, {}", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    }  
}

void CTPMarketGateWay::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
    if (bResult)
    {
        m_Logger->Log->warn("CTPMarketGateWay::OnRspError 返回错误，ErrorID={}, {}", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    }
}

void CTPMarketGateWay::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
    if (!bResult && pSpecificInstrument != NULL)
    {
        m_Logger->Log->info("CTPMarketGateWay::OnRspSubMarketData 订阅行情成功 合约代码:{}", pSpecificInstrument->InstrumentID);
    }
    else
    {
        m_Logger->Log->warn("CTPMarketGateWay::OnRspSubMarketData 返回错误，ErrorID={}, {}", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    }
}

void CTPMarketGateWay::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
    m_Logger->Log->debug("CTPMarketGateWay::OnRspUnSubMarketData ErrorID={}, ErrorMsg={} nRequestID={} bIsLast={}", pRspInfo->ErrorID, pRspInfo->ErrorMsg, nRequestID, bIsLast);
    if (!bResult)
    {
        m_Logger->Log->info("CTPMarketGateWay::OnRspUnSubMarketData 取消订阅行情成功 合约代码:{}", pSpecificInstrument->InstrumentID);
    }
    else
    {
        m_Logger->Log->warn("CTPMarketGateWay::OnRspUnSubMarketData 返回错误，ErrorID={}, {}", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    }
}

void CTPMarketGateWay::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData)
{
    // print Market Data for Debug
    // PrintDepthMarketData(pDepthMarketData);
    // 处理订阅合约行情数据
    auto it = m_TickerExchangeMap.find(pDepthMarketData->InstrumentID);
    if (it != m_TickerExchangeMap.end())
    {
        memset(&m_MarketData, 0, sizeof(m_MarketData));
        strncpy(m_MarketData.ExchangeID, it->second.c_str(), sizeof(m_MarketData.ExchangeID));
        ParseMarketData(*pDepthMarketData, m_MarketData);
        // 写入行情数据到行情队列
        Message::PackMessage message;
        message.MessageType = Message::EMessageType::EFutureMarketData;
        memcpy(&message.FutureMarketData, &m_MarketData, sizeof(message.FutureMarketData));
        while(!m_MarketMessageQueue.Push(message));
        m_Logger->Log->debug("CTPMarketGateWay::OnRtnDepthMarketData Ticker:{} UpdateTime:{}", 
                            message.FutureMarketData.Ticker, message.FutureMarketData.UpdateTime);
    }
}

void CTPMarketGateWay::ParseMarketData(const CThostFtdcDepthMarketDataField& depthMarketData, MarketData::TFutureMarketData &tickData)
{
    // numeric_limits<double>::max()
    strncpy(tickData.Ticker, depthMarketData.InstrumentID, sizeof(tickData.Ticker));
    strncpy(tickData.TradingDay, depthMarketData.TradingDay, sizeof(tickData.TradingDay));
    strncpy(tickData.ActionDay, depthMarketData.ActionDay, sizeof(tickData.ActionDay));
    strncpy(tickData.UpdateTime, depthMarketData.UpdateTime, sizeof(tickData.UpdateTime));
    tickData.MillSec = depthMarketData.UpdateMillisec;
    tickData.LastPrice = depthMarketData.LastPrice;
    tickData.Volume = depthMarketData.Volume;
    tickData.Turnover = depthMarketData.Turnover;
    tickData.OpenPrice = depthMarketData.OpenPrice;
    tickData.ClosePrice = depthMarketData.ClosePrice;
    tickData.PreClosePrice = depthMarketData.PreClosePrice;
    tickData.SettlementPrice = depthMarketData.SettlementPrice;
    tickData.PreSettlementPrice = depthMarketData.PreSettlementPrice;
    tickData.OpenInterest = depthMarketData.OpenInterest;
    tickData.PreOpenInterest = depthMarketData.PreOpenInterest;
    tickData.CurrDelta = depthMarketData.CurrDelta;
    tickData.PreDelta = depthMarketData.PreDelta;
    
    tickData.HighestPrice = depthMarketData.HighestPrice;
    tickData.LowestPrice = depthMarketData.LowestPrice;
    tickData.UpperLimitPrice = depthMarketData.UpperLimitPrice;
    tickData.LowerLimitPrice = depthMarketData.LowerLimitPrice;

    tickData.AveragePrice = depthMarketData.AveragePrice;

    tickData.BidPrice1 = depthMarketData.BidPrice1;
    tickData.BidVolume1 = depthMarketData.BidVolume1;
    tickData.AskPrice1 = depthMarketData.AskPrice1;
    tickData.AskVolume1 = depthMarketData.AskVolume1;

    tickData.BidPrice2 = depthMarketData.BidPrice2;
    tickData.BidVolume2 = depthMarketData.BidVolume2;
    tickData.AskPrice2 = depthMarketData.AskPrice2;
    tickData.AskVolume2 = depthMarketData.AskVolume2;

    tickData.BidPrice3 = depthMarketData.BidPrice3;
    tickData.BidVolume3 = depthMarketData.BidVolume3;
    tickData.AskPrice3 = depthMarketData.AskPrice3;
    tickData.AskVolume3 = depthMarketData.AskVolume3;

    tickData.BidPrice4 = depthMarketData.BidPrice4;
    tickData.BidVolume4 = depthMarketData.BidVolume4;
    tickData.AskPrice4 = depthMarketData.AskPrice4;
    tickData.AskVolume4 = depthMarketData.AskVolume4;

    tickData.BidPrice5 = depthMarketData.BidPrice5;
    tickData.BidVolume5 = depthMarketData.BidVolume5;
    tickData.AskPrice5 = depthMarketData.AskPrice5;
    tickData.AskVolume5 = depthMarketData.AskVolume5;
}

void CTPMarketGateWay::PrintDepthMarketData(const CThostFtdcDepthMarketDataField *pDepthMarketData)
{
    m_Logger->Log->debug("====================================={}===============================", pDepthMarketData->InstrumentID);
    m_Logger->Log->debug("TradingDay {}", pDepthMarketData->TradingDay);
    m_Logger->Log->debug("ExchangeID {}", pDepthMarketData->ExchangeID);
    m_Logger->Log->debug("Ticker {}", pDepthMarketData->InstrumentID);
    m_Logger->Log->debug("ExchangeInstID {}", pDepthMarketData->ExchangeInstID);
    m_Logger->Log->debug("LastPrice {}", pDepthMarketData->LastPrice);
    m_Logger->Log->debug("Volume {}", pDepthMarketData->Volume);
    m_Logger->Log->debug("PreSettlementPrice {}", pDepthMarketData->PreSettlementPrice);
    m_Logger->Log->debug("PreClosePrice {}", pDepthMarketData->PreClosePrice);
    m_Logger->Log->debug("PreOpenInterest {}", pDepthMarketData->PreOpenInterest);
    m_Logger->Log->debug("OpenPrice {}", pDepthMarketData->OpenPrice);
    m_Logger->Log->debug("HighestPrice {}", pDepthMarketData->HighestPrice);
    m_Logger->Log->debug("LowestPrice {}", pDepthMarketData->LowestPrice);
    m_Logger->Log->debug("Turnover {}", pDepthMarketData->Turnover);
    m_Logger->Log->debug("OpenInterest {}", pDepthMarketData->OpenInterest);
    m_Logger->Log->debug("ClosePrice {}", pDepthMarketData->ClosePrice);
    m_Logger->Log->debug("SettlementPrice {}", pDepthMarketData->SettlementPrice);
    m_Logger->Log->debug("UpperLimitPrice {}", pDepthMarketData->UpperLimitPrice);
    m_Logger->Log->debug("PreDelta {}", pDepthMarketData->PreDelta);
    m_Logger->Log->debug("CurrDelta {}", pDepthMarketData->CurrDelta);
    m_Logger->Log->debug("UpdateTime {}", pDepthMarketData->UpdateTime);
    m_Logger->Log->debug("UpdateMillisec {}", pDepthMarketData->UpdateMillisec);
    m_Logger->Log->debug("BidPrice1 {}", pDepthMarketData->BidPrice1);
    m_Logger->Log->debug("BidVolume1 {}", pDepthMarketData->BidVolume1);
    m_Logger->Log->debug("AskPrice1 {}", pDepthMarketData->AskPrice1);
    m_Logger->Log->debug("AskVolume1 {}", pDepthMarketData->AskVolume1);
    m_Logger->Log->debug("BidPrice2 {}", pDepthMarketData->BidPrice2);
    m_Logger->Log->debug("BidVolume2 {}", pDepthMarketData->BidVolume2);
    m_Logger->Log->debug("AskPrice2 {}", pDepthMarketData->AskPrice2);
    m_Logger->Log->debug("AskVolume2 {}", pDepthMarketData->AskVolume2);
    m_Logger->Log->debug("BidPrice3 {}", pDepthMarketData->BidPrice3);
    m_Logger->Log->debug("BidVolume3 {}", pDepthMarketData->BidVolume3);
    m_Logger->Log->debug("AskPrice3 {}", pDepthMarketData->AskPrice3);
    m_Logger->Log->debug("AskVolume3 {}", pDepthMarketData->AskVolume3);
    m_Logger->Log->debug("BidPrice4 {}", pDepthMarketData->BidPrice4);
    m_Logger->Log->debug("BidVolume4 {}", pDepthMarketData->BidVolume4);
    m_Logger->Log->debug("AskPrice4 {}", pDepthMarketData->AskPrice4);
    m_Logger->Log->debug("AskVolume4 {}", pDepthMarketData->AskVolume4);
    m_Logger->Log->debug("BidPrice5 {}", pDepthMarketData->BidPrice5);
    m_Logger->Log->debug("BidVolume5 {}", pDepthMarketData->BidVolume5);
    m_Logger->Log->debug("AskPrice5 {}", pDepthMarketData->AskPrice5);
    m_Logger->Log->debug("AskVolume5 {}", pDepthMarketData->AskVolume5);
    m_Logger->Log->debug("AveragePrice {}", pDepthMarketData->AveragePrice);
    m_Logger->Log->debug("ActionDay {}", pDepthMarketData->ActionDay);
}