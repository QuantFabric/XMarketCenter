#ifndef CTPMARKETDATALOGGER_HPP
#define CTPMARKETDATALOGGER_HPP

#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/pattern_formatter.h"
#include "spdlog/async.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <string>
#include "MarketData.hpp"
#include "Singleton.hpp"

class CTPMarketDataLogger
{
    friend Utils::Singleton<CTPMarketDataLogger>;
public:
    void WriteFutureMarketData(const MarketData::TFutureMarketData &data)
    {
        if(nullptr == m_DataLogger)
        {
            Init(data.ExchangeID, data.TradingDay);
        }
        WriteMarketDataFile(data);
    }

protected:
    void WriteMarketDataFile(const MarketData::TFutureMarketData &data)
    {
        m_DataLogger->info("{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}",
            data.Ticker,
            data.ExchangeID,
            data.ActionDay,
            data.UpdateTime,
            data.MillSec,
            data.LastPrice,
            data.PreSettlementPrice,
            data.PreClosePrice,
            data.Volume,
            data.Turnover,
            data.OpenInterest,
            data.OpenPrice,
            data.HighestPrice,
            data.LowestPrice,
            data.UpperLimitPrice,
            data.LowerLimitPrice,
            data.BidPrice1,
            data.BidVolume1,
            data.AskPrice1,
            data.AskVolume1,
            data.BidPrice2,
            data.BidVolume2,
            data.AskPrice2,
            data.AskVolume2,
            data.BidPrice3,
            data.BidVolume3,
            data.AskPrice3,
            data.AskVolume3,
            data.BidPrice4,
            data.BidVolume4,
            data.AskPrice4,
            data.AskVolume4,
            data.BidPrice5,
            data.BidVolume5,
            data.AskPrice5,
            data.AskVolume5);
    }

    void FormatMarketDataHeader(const std::string& delimiter, std::string& out)
    {
        std::string field =
            "Ticker" + delimiter +
            "ExchangeID" + delimiter +
            "ActionDay" + delimiter +
            "UpdateTime" + delimiter +
            "MillSec" + delimiter +
            "LastPrice" + delimiter +
            "PreSettlementPrice" + delimiter +
            "PreClosePrice" + delimiter +
            "Volume" + delimiter +
            "Turnover" + delimiter +
            "OpenInterest" + delimiter +
            "OpenPrice" + delimiter +
            "HighestPrice" + delimiter +
            "LowestPrice" + delimiter +
            "UpperLimitPrice" + delimiter +
            "LowerLimitPrice" + delimiter +
            "BidPrice1" + delimiter +
            "BidVolume1" + delimiter +
            "AskPrice1" + delimiter +
            "AskVolume1" + delimiter +
            "BidPrice2" + delimiter +
            "BidVolume2" + delimiter +
            "AskPrice2" + delimiter +
            "AskVolume2" + delimiter +
            "BidPrice3" + delimiter +
            "BidVolume3" + delimiter +
            "AskPrice3" + delimiter +
            "AskVolume3" + delimiter +
            "BidPrice4" + delimiter +
            "BidVolume4" + delimiter +
            "AskPrice4" + delimiter +
            "AskVolume4" + delimiter +
            "BidPrice5" + delimiter +
            "BidVolume5" + delimiter +
            "AskPrice5" + delimiter +
            "AskVolume5";
        out = field;
    }

    void Init(const std::string& ExchangeID, const std::string& tradingday)
    {
        std::string data_log_path = getenv("DATA_LOG_PATH");
        std::string log_name = data_log_path + "/" + ExchangeID + "_" + tradingday + ".csv";
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_name);
        file_sink->set_pattern("%v");
        auto logger = std::make_shared<spdlog::logger>(ExchangeID, file_sink);
        // 文件已经存在，不需要写header
        if(0 != access(log_name.c_str(), F_OK))
        {
            std::string marketHeader;
            FormatMarketDataHeader(",", marketHeader);
            logger->info(marketHeader); 
        }
        m_DataLogger = logger;
    }

private:
    CTPMarketDataLogger() {}
    CTPMarketDataLogger &operator=(const CTPMarketDataLogger &);
    CTPMarketDataLogger(const CTPMarketDataLogger &);
private:
    std::shared_ptr<spdlog::logger> m_DataLogger;
};


#endif // CTPMARKETDATALOGGER_HPP