#include "REMMarketGateWay.h"
#include "XPluginEngine.hpp"

CreateObjectFunc(REMMarketGateWay);

REMMarketGateWay::REMMarketGateWay() : MarketGateWay()
{

}

bool REMMarketGateWay::LoadAPIConfig()
{
    bool ret = true;
    std::string errorBuffer;
    if(Utils::LoadREMMarkeSourceConfig(m_MarketCenterConfig.APIConfig.c_str(), m_REMMarketSourceConfig, errorBuffer))
    {
        m_Logger->Log->info("REMMarketGateWay::LoadREMMarkeSourceConfig {} successed", m_MarketCenterConfig.APIConfig);
        m_Logger->Log->info("REMMarketGateWay Account:{} Password:{} FrontIP:{} FrontPort:{} MultiCastIP:{} MultiCastPort:{} LocalIP:{} LocalPort:{} TickerListPath:{}", 
                            m_REMMarketSourceConfig.Account, m_REMMarketSourceConfig.Password,
                            m_REMMarketSourceConfig.FrontIP, m_REMMarketSourceConfig.FrontPort,
                            m_REMMarketSourceConfig.MultiCastIP, m_REMMarketSourceConfig.MultiCastPort,
                            m_REMMarketSourceConfig.LocalIP, m_REMMarketSourceConfig.LocalPort,
                            m_REMMarketSourceConfig.TickerListPath);
    }
    else
    {
        ret = false;
        m_Logger->Log->error("REMMarketGateWay::LoadREMMarkeSourceConfig {} failed, {}", m_MarketCenterConfig.APIConfig, errorBuffer);
    }
    return ret;
}

void REMMarketGateWay::Run()
{
    m_pEESQuoteApi = CreateEESQuoteApi();
    if(m_pEESQuoteApi == NULL)
    {
        m_Logger->Log->error("REMMarketGateWay::Run CreateEESQuoteApi failed");
        return;
    }
    if(true) // TCP 
    {
        EqsTcpInfo addressInfo;
        memset(&addressInfo, 0, sizeof(addressInfo));
        strncpy(addressInfo.m_eqsIp, m_REMMarketSourceConfig.FrontIP.c_str(), sizeof(addressInfo.m_eqsIp));
        addressInfo.m_eqsPort = m_REMMarketSourceConfig.FrontPort;
        std::vector<EqsTcpInfo> TCPVector;
        TCPVector.push_back(addressInfo);
        m_pEESQuoteApi->ConnServer(TCPVector, this);
    }
    else  // UDP MultiCast
    {
        EqsMulticastInfo MultiCastAddress;
        memset(&MultiCastAddress, 0, sizeof(MultiCastAddress));
        strncpy(MultiCastAddress.m_mcIp, m_REMMarketSourceConfig.MultiCastIP.c_str(), sizeof(MultiCastAddress.m_mcIp));
        MultiCastAddress.m_mcPort = m_REMMarketSourceConfig.MultiCastPort;
        strncpy(MultiCastAddress.m_mcLoacalIp, m_REMMarketSourceConfig.LocalIP.c_str(), sizeof(MultiCastAddress.m_mcLoacalIp));
        MultiCastAddress.m_mcLocalPort = m_REMMarketSourceConfig.LocalPort;
        // CFFEX, DCE, CZCE, SHFE, SHH, SHZ
        strncpy(MultiCastAddress.m_exchangeId, m_MarketCenterConfig.ExchangeID.c_str(), sizeof(MultiCastAddress.m_exchangeId));
        std::vector<EqsMulticastInfo> UDPVector;
        UDPVector.push_back(MultiCastAddress);
        m_pEESQuoteApi->InitMulticast(UDPVector, this);
    }
}

void REMMarketGateWay::GetCommitID(std::string& CommitID, std::string& UtilsCommitID)
{
    CommitID = SO_COMMITID;
    UtilsCommitID = SO_UTILS_COMMITID;
}

void REMMarketGateWay::GetAPIVersion(std::string& APIVersion)
{
    APIVersion = API_VERSION;
}

void REMMarketGateWay::ParseMarketData(const EESMarketDepthQuoteData& DepthQuoteData, MarketData::TFutureMarketData& tickData)
{
    strncpy(tickData.Ticker, DepthQuoteData.InstrumentID, sizeof(tickData.Ticker));
    strncpy(tickData.TradingDay, DepthQuoteData.TradingDay, sizeof(tickData.TradingDay));
    strncpy(tickData.ActionDay, Utils::getCurrentDay(), sizeof(tickData.ActionDay));
    strncpy(tickData.UpdateTime, DepthQuoteData.UpdateTime, sizeof(tickData.UpdateTime));
    strncpy(tickData.ExchangeID, DepthQuoteData.ExchangeID, sizeof(tickData.ExchangeID));
    tickData.MillSec = DepthQuoteData.UpdateMillisec;
    tickData.LastPrice = DepthQuoteData.LastPrice;
    tickData.Volume = DepthQuoteData.Volume;
    tickData.Turnover = DepthQuoteData.Turnover;
    tickData.PreSettlementPrice = DepthQuoteData.PreSettlementPrice;
    tickData.PreClosePrice = DepthQuoteData.PreClosePrice;
    tickData.OpenInterest = DepthQuoteData.OpenInterest;
    tickData.OpenPrice = DepthQuoteData.OpenPrice;
    tickData.HighestPrice = DepthQuoteData.HighestPrice;
    tickData.LowestPrice = DepthQuoteData.LowestPrice;
    tickData.UpperLimitPrice = DepthQuoteData.UpperLimitPrice;
    tickData.LowerLimitPrice = DepthQuoteData.LowerLimitPrice;

    tickData.BidPrice1 = DepthQuoteData.BidPrice1;
    tickData.BidVolume1 = DepthQuoteData.BidVolume1;
    tickData.AskPrice1 = DepthQuoteData.AskPrice1;
    tickData.AskVolume1 = DepthQuoteData.AskVolume1;

    tickData.BidPrice2 = DepthQuoteData.BidPrice2;
    tickData.BidVolume2 = DepthQuoteData.BidVolume2;
    tickData.AskPrice2 = DepthQuoteData.AskPrice2;
    tickData.AskVolume2 = DepthQuoteData.AskVolume2;

    tickData.BidPrice3 = DepthQuoteData.BidPrice3;
    tickData.BidVolume3 = DepthQuoteData.BidVolume3;
    tickData.AskPrice3 = DepthQuoteData.AskPrice3;
    tickData.AskVolume3 = DepthQuoteData.AskVolume3;

    tickData.BidPrice4 = DepthQuoteData.BidPrice4;
    tickData.BidVolume4 = DepthQuoteData.BidVolume4;
    tickData.AskPrice4 = DepthQuoteData.AskPrice4;
    tickData.AskVolume4 = DepthQuoteData.AskVolume4;

    tickData.BidPrice5 = DepthQuoteData.BidPrice5;
    tickData.BidVolume5 = DepthQuoteData.BidVolume5;
    tickData.AskPrice5 = DepthQuoteData.AskPrice5;
    tickData.AskVolume5 = DepthQuoteData.AskVolume5;
}

void REMMarketGateWay::PrintDepthMarketData(const EESMarketDepthQuoteData* pDepthMarketData)
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
}

void REMMarketGateWay::OnEqsConnected()
{
    EqsLoginParam loginParam;
    memset(&loginParam, 0, sizeof(loginParam));
    strncpy(loginParam.m_loginId, m_REMMarketSourceConfig.Account.c_str(), sizeof(loginParam.m_loginId));
    strncpy(loginParam.m_password, m_REMMarketSourceConfig.Password.c_str(), sizeof(loginParam.m_password));
    m_pEESQuoteApi->LoginToEqs(loginParam);
}

void REMMarketGateWay::OnEqsDisconnected()
{
    m_Logger->Log->warn("REMMarketGateWay::OnEqsDisconnected");
}

void REMMarketGateWay::OnLoginResponse(bool bSuccess, const char* pReason)
{
    m_Logger->Log->info("REMMarketGateWay::OnLoginResponse");
    for(auto it = m_TickerPropertyList.begin(); it != m_TickerPropertyList.end(); it++)
    {
        m_pEESQuoteApi->RegisterSymbol(EesEqsIntrumentType::EQS_FUTURE, it->Ticker.c_str());
    }
}

void REMMarketGateWay::OnQuoteUpdated(EesEqsIntrumentType chInstrumentType, EESMarketDepthQuoteData* pDepthQuoteData)
{
    if(EesEqsIntrumentType::EQS_FUTURE == chInstrumentType)
    {
        std::string Ticker = pDepthQuoteData->InstrumentID;
        auto it = m_TickerExchangeMap.find(Ticker);
        if(it != m_TickerExchangeMap.end())
        {
            ParseMarketData(*pDepthQuoteData, m_MarketData);
            Message::PackMessage message;
            memset(&message, 0, sizeof(message));
            message.MessageType = Message::EMessageType::EFutureMarketData;
            memcpy(&message.FutureMarketData, &m_MarketData, sizeof(message.FutureMarketData));
            while(!m_MarketMessageQueue.Push(message));
        }
    }
}

void REMMarketGateWay::OnWriteTextLog(EesEqsLogLevel nlevel, const char* pLogText, int nLogLen)
{
    if(EesEqsLogLevel::QUOTE_LOG_LV_DEBUG == nlevel)
    {
        m_Logger->Log->debug("REMMarketGateWay::OnWriteTextLog {}", pLogText);
    }
    else if(EesEqsLogLevel::QUOTE_LOG_LV_INFO == nlevel)
    {
        m_Logger->Log->info("REMMarketGateWay::OnWriteTextLog {}", pLogText);
    }
    else if(EesEqsLogLevel::QUOTE_LOG_LV_WARN == nlevel)
    {
        m_Logger->Log->warn("REMMarketGateWay::OnWriteTextLog {}", pLogText);
    }
    else if(EesEqsLogLevel::QUOTE_LOG_LV_ERROR == nlevel)
    {
        m_Logger->Log->error("REMMarketGateWay::OnWriteTextLog {}", pLogText);
    }
    else if(EesEqsLogLevel::QUOTE_LOG_LV_FATAL == nlevel)
    {
        m_Logger->Log->error("REMMarketGateWay::OnWriteTextLog {}", pLogText);
    }
}

void REMMarketGateWay::OnSymbolRegisterResponse(EesEqsIntrumentType chInstrumentType, const char* pSymbol, bool bSuccess)
{
    m_Logger->Log->info("REMMarketGateWay::OnSymbolRegisterResponse Ticker:{} bSuccess:{}", pSymbol, bSuccess);
    if(!bSuccess)
    {
        m_pEESQuoteApi->RegisterSymbol(chInstrumentType, pSymbol);
    }
}

void REMMarketGateWay::OnSymbolUnregisterResponse(EesEqsIntrumentType chInstrumentType, const char* pSymbol, bool bSuccess)
{
    m_Logger->Log->info("REMMarketGateWay::OnSymbolUnregisterResponse Ticker:{} bSuccess:{}", pSymbol, bSuccess);
}

void REMMarketGateWay::OnSymbolListResponse(EesEqsIntrumentType chInstrumentType, const char* pSymbol, bool bLast)
{
    m_Logger->Log->info("REMMarketGateWay::OnSymbolListResponse Ticker:{} bSuccess:{}", pSymbol, bLast);
}   