#ifndef CRAWLERMARKETGATEWAY_HPP
#define CRAWLERMARKETGATEWAY_HPP

#include <string>
#include <algorithm>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "MarketGateWay.hpp"

class CrawlerMarketGateWay : public MarketGateWay
{
public:
    virtual void Run()
    {
        m_Logger->Log->info("CrawlerMarketGateWay::Run Crawler {} {}", m_Source, m_Script);
        while(true)
        {
            std::vector<std::string> result;
            int ret = ExecuteShellCommand(m_Script, result);
            if(ret > 0)
            {
                for(int i = 0; i < result.size(); i++)
                {
                    std::vector<std::string> fields;
                    Utils::Split(result.at(i), ",", fields);
                    MarketData::TFutureMarketData data;
                    bool ok = ParseMarketData(fields, data);
                    if(ok)
                    {
                        auto it = m_TickerExchangeMap.find(data.Ticker);
                        if(it != m_TickerExchangeMap.end())
                        {
                            Message::PackMessage message;
                            message.MessageType = Message::EMessageType::ESpotMarketData;
                            memcpy(&message.FutureMarketData, &data, sizeof(message.FutureMarketData));
                            strncpy(message.FutureMarketData.ExchangeID, it->second.c_str(), sizeof(message.FutureMarketData.ExchangeID));
                            m_MarketMessageQueue.push(message);
                        }
                    }
                    m_Logger->Log->debug("CrawlerMarketGateWay::Run line:{}", result.at(i));
                }
            }
            usleep(m_TimeOut * 1000);
        }
    }
protected:
    int ExecuteShellCommand(const std::string& script, std::vector<std::string>& result)
    {
        int ret = -1;
        result.clear();
        FILE *pp = popen(script.c_str(), "r"); //建立管道
        if (pp != NULL)
        {
            char buffer[1024]; //设置一个合适的长度，以存储每一行输出
            while (fgets(buffer, sizeof(buffer), pp) != NULL)
            {
                if (buffer[strlen(buffer) - 1] == '\n')
                {
                    buffer[strlen(buffer) - 1] = '\0'; //去除换行符
                }
                result.push_back(buffer);
            }
            pclose(pp); //关闭管道
        }
        else
        {
            printf("%s\n", script.c_str());
        }
        ret = result.size();
        return ret;
    }

    virtual bool ParseMarketData(const std::vector<std::string>& market, MarketData::TFutureMarketData &data) = 0;
protected:
    int m_TimeOut;
    std::string m_Script;
    std::string m_Source;
};


#endif // CRAWLERMARKETGATEWAY_HPP