#ifndef REMMARKETGATEWAY_H
#define REMMARKETGATEWAY_H

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string.h>
#include <stdio.h>
#include "EESQuoteApi.h"
#include "YMLConfig.hpp"
#include "Logger.h"
#include "MarketGateWay.hpp"
#include "PackMessage.hpp"

class REMMarketGateWay : public MarketGateWay, public EESQuoteEvent
{
public:
    explicit REMMarketGateWay();
public:
    virtual bool LoadAPIConfig();
    virtual void Run();
    virtual void GetCommitID(std::string& CommitID, std::string& UtilsCommitID);
    virtual void GetAPIVersion(std::string& APIVersion);
protected:
    void ParseMarketData(const EESMarketDepthQuoteData& DepthQuoteData, MarketData::TFutureMarketData& tickData);
    void PrintDepthMarketData(const EESMarketDepthQuoteData* pDepthQuoteData);
private:
    MarketData::TFutureMarketData m_MarketData;
    EESQuoteApi *m_pEESQuoteApi;
    Utils::REMMarketSourceConfig m_REMMarketSourceConfig;
public:
    /*********************************************************************************
     *  API callback implementation
     * ******************************************************************************/
	// 当服务器连接成功，登录前调用, 如果是组播模式不会发生, 只需判断InitMulticast返回值即可
    virtual void OnEqsConnected();

	// 当服务器曾经连接成功，被断开时调用，组播模式不会发生该事件
    virtual void OnEqsDisconnected();

	// 当登录成功或者失败时调用，组播模式不会发生
	// bSuccess 登陆是否成功标志  
	// pReason  登陆失败原因  
    virtual void OnLoginResponse(bool bSuccess, const char* pReason);

	// 收到行情时调用,具体格式根据instrument_type不同而不同
	// chInstrumentType  EES行情类型
	// pDepthQuoteData   EES统一行情指针  
    virtual void OnQuoteUpdated(EesEqsIntrumentType chInstrumentType, EESMarketDepthQuoteData* pDepthQuoteData);

	// 日志接口，让使用者帮助写日志。
	// nlevel    日志级别
	// pLogText  日志内容
	// nLogLen   日志长度
    virtual void OnWriteTextLog(EesEqsLogLevel nlevel, const char* pLogText, int nLogLen);

	// 注册symbol响应消息来时调用，组播模式不支持行情注册
	// chInstrumentType  EES行情类型
	// pSymbol           合约名称
	// bSuccess          注册是否成功标志
    virtual void OnSymbolRegisterResponse(EesEqsIntrumentType chInstrumentType, const char* pSymbol, bool bSuccess);

	// 注销symbol响应消息来时调用，组播模式不支持行情注册
	// chInstrumentType  EES行情类型
	// pSymbol           合约名称
	// bSuccess          注册是否成功标志
    virtual void OnSymbolUnregisterResponse(EesEqsIntrumentType chInstrumentType, const char* pSymbol, bool bSuccess);
	
	// 查询symbol列表响应消息来时调用，组播模式不支持合约列表查询
	// chInstrumentType  EES行情类型
	// pSymbol           合约名称
	// bLast             最后一条查询合约列表消息的标识
	// 查询合约列表响应, last = true时，本条数据是无效数据。
    virtual void OnSymbolListResponse(EesEqsIntrumentType chInstrumentType, const char* pSymbol, bool bLast);
};

#endif // REMMARKETGATEWAY_H