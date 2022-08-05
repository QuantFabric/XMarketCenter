#ifndef XUEQIUMARKETGATEWAY_H
#define XUEQIUMARKETGATEWAY_H

#include <string>
#include <algorithm>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "YMLConfig.hpp"
#include "MarketData.hpp"
#include "Singleton.hpp"
#include "Logger.h"
#include "CrawlerMarketGateWay.hpp"

class XueQiuMarketGateWay : public CrawlerMarketGateWay
{
public:
    explicit XueQiuMarketGateWay();
    virtual ~XueQiuMarketGateWay();
public:
    virtual bool LoadAPIConfig();
    virtual void GetCommitID(std::string& CommitID, std::string& UtilsCommitID);
    virtual void GetAPIVersion(std::string& APIVersion);
protected:
    virtual bool ParseMarketData(const std::vector<std::string> &market, MarketData::TFutureMarketData &data);
protected:
    Utils::CrawlerMarketSourceConfig m_CrawlerMarketSourceConfig;
};

#endif //XUEQIUMARKETGATEWAY_H