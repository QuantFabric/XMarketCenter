#ifndef RAWMARKETGATEWAY_HPP
#define RAWMARKETGATEWAY_HPP

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string>
#include <net/if.h>
#include <string>

#include "MarketData.hpp"
#include "YMLConfig.hpp"
#include "MarketGateWay.hpp"

class RawMarketGateWay : public MarketGateWay
{
public:
    virtual void Run()
    {
        m_PacketSocket = socket(PF_PACKET, SOCK_DGRAM, htons(ETH_P_IP));
        m_Logger->Log->info("RawMarketGateWay::Run Capture {} Packet m_IPDiagramLen:{} m_MessageLen:{} m_PacketSocket:{}", 
                            m_Device, m_IPDiagramLen, m_MessageLen, m_PacketSocket);
        bool ret = true;
        ret = ret && SetPromisc(m_PacketSocket, m_Device.c_str(), 1);
        ret = ret && BindDevice(m_PacketSocket, m_Device.c_str());
        if(!ret)
        {
            perror("RawMarketGateWay::Run");
            close(m_PacketSocket);
            return;
        }
        uint8_t m_IPDatagramBuffer[m_IPDiagramLen];
        uint8_t m_MessageBuffer[m_MessageLen];
        static std::unordered_set<std::string> TickerSet;
        std::string Ticker;
        while (true)
        {
            int ret = read(m_PacketSocket, m_IPDatagramBuffer, m_IPDiagramLen);
            if (ret < 0)
            {
                m_Logger->Log->error("RawMarketGateWay::Run Read Datagram Error {}", ret);
                sleep(1);
                return;
            }
            else if(ret == m_IPDiagramLen)
            {
                memcpy(m_MessageBuffer, m_IPDatagramBuffer + (m_IPDiagramLen - m_MessageLen), m_MessageLen);
                ParseTicker(m_MessageBuffer, Ticker);
                auto it = m_TickerPropertyMap.find(Ticker);
                if (it != m_TickerPropertyMap.end())
                {
                    // parse
                    memset(&m_MarketData, 0, sizeof(m_MarketData));
                    memcpy(m_MarketData.RevDataLocalTime, Utils::getCurrentTimeUs(), sizeof(m_MarketData.RevDataLocalTime));
                    ParseMarketData(m_MessageBuffer, it->second.PriceTick, m_MarketData);
                    std::string datetime = Utils::getCurrentDay();
                    datetime = datetime + " " + m_MarketData.UpdateTime;
                    memcpy(m_MarketData.UpdateTime, datetime.c_str(), sizeof(m_MarketData.UpdateTime));
                    Utils::CalculateTick(m_MarketCenterConfig, m_MarketData);
                    static long Tick = -1;
                    if(Tick < m_MarketData.Tick)
                    {
                        Tick = m_MarketData.Tick;
                        TickerSet.clear();
                    }
                    auto ticker_it = TickerSet.find(Ticker);
                    if(ticker_it == TickerSet.end())
                    {
                        // PrintMarketData(m_MarketData);
                        // 写入订阅合约行情数据到期货行情队列
                        Message::PackMessage message;
                        message.MessageType = Message::EMessageType::EFutureMarketData;
                        memcpy(&message.FutureMarketData, &m_MarketData, sizeof(message.FutureMarketData));
                        strncpy(message.FutureMarketData.ExchangeID, it->second.ExchangeID.c_str(), sizeof(message.FutureMarketData.ExchangeID));
                        m_MarketMessageQueue.push(message);
                        TickerSet.insert(Ticker);
                    }
                    m_Logger->Log->debug("RawMarketGateWay::Run Ticker:{} Tick:{}", Ticker, m_MarketData.Tick);
                }
            }
        }
    }
protected:
    virtual bool ParseMarketData(uint8_t *buffer, double PriceTick, MarketData::TFutureMarketData& data) = 0;
    virtual void ParseTicker(uint8_t *buffer, std::string& Ticker) = 0;
protected:
    // 网卡混杂模式设置
    bool SetPromisc(int sockfd, const char *szIfName, int iFlags)
    {
        bool ret = false;
        ifreq stIfr;
        strcpy(stIfr.ifr_name, szIfName);
        if(ioctl(sockfd, SIOCGIFFLAGS, &stIfr) >= 0)
        {
            stIfr.ifr_flags = (iFlags) ? (stIfr.ifr_flags | IFF_PROMISC) : (stIfr.ifr_flags & ~IFF_PROMISC);
            if(ioctl(sockfd, SIOCSIFFLAGS, &stIfr) >= 0)
            {
                ret = true;
            }
        }
        m_Logger->Log->info("RawMarketGateWay::SetPromisc {} Socket:{} ret:{}", szIfName, sockfd, ret);
        return ret;
    }
    // 网卡绑定
    bool BindDevice(int sockfd, const char* device)
    {
        bool ret = false;
        struct sockaddr_ll sl;
        struct ifreq ifr;
        memset(&sl, 0, sizeof(sl));
        memset(&ifr, 0, sizeof(ifr));
        sl.sll_family = PF_PACKET;
        sl.sll_protocol = htons(ETH_P_IP);
        strncpy(ifr.ifr_name, device, sizeof(ifr.ifr_name));
        if(ioctl(sockfd, SIOCGIFINDEX, &ifr) >= 0)
        {
            sl.sll_ifindex = ifr.ifr_ifindex;
            ret = bind(sockfd, (struct sockaddr *)&sl, sizeof(sl)) == 0;
        }
        m_Logger->Log->info("RawMarketGateWay::BindDevice {} Socket:{} ret:{}", device, sockfd, ret);
        return ret;
    }
protected:
    std::string m_Device;
    int m_IPDiagramLen;
    int m_MessageLen;
    int m_PacketSocket;
    MarketData::TFutureMarketData m_MarketData;
};


#endif // RAWMARKETGATEWAY_HPP