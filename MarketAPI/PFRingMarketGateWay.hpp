#ifndef PFRINGMARKETGATEWAY_HPP
#define PFRINGMARKETGATEWAY_HPP

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <net/if.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <time.h>
#include <sys/time.h>

#include "MarketData.hpp"
#include "YMLConfig.hpp"
#include "RawMarketGateWay.hpp"

class PFRingMarketGateWay : public RawMarketGateWay
{
public:
    virtual void Run()
    {
        m_PacketSocket = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
        m_Logger->Log->info("PFRingMarketGateWay::Run Capture Packet m_IPDiagramLen:{} m_MessageLen:{} m_PacketSocket:{}", 
                            m_IPDiagramLen, m_MessageLen, m_PacketSocket);
        bool ret = true;
        ret = ret && SetPromisc(m_PacketSocket, m_Device.c_str(), 1);
        ret = ret && BindDevice(m_PacketSocket, m_Device.c_str());
        ret = ret && InitRingBuffer(m_PacketSocket);
        if(!ret)
        {
            perror("PFRingMarketGateWay::Run");
            close(m_PacketSocket);
        }
        uint8_t m_IPDatagramBuffer[m_IPDiagramLen];
        uint8_t m_MessageBuffer[m_MessageLen];
        std::unordered_set<std::string> TickerSet;
        std::string Ticker;
        int HeaderLen = m_IPDiagramLen - m_MessageLen;
        while(true)
        {
            for(unsigned int index = 0; index < m_PacketRequest.tp_frame_nr; ++index)
            {		
                tpacket_hdr *pHead = (tpacket_hdr *)(m_MMAPAddr + index * m_PacketRequest.tp_frame_size);
                while (TP_STATUS_KERNEL == pHead->tp_status)
                	usleep(10);
                unsigned char *pIPPacket = (unsigned char *)pHead + pHead->tp_net;
                iphdr* pIPHeader = (iphdr*)(pIPPacket);
                int IPPacketLen = ntohs(pIPHeader->tot_len);
                if(pIPHeader->protocol == 0x11 && IPPacketLen == m_IPDiagramLen)
                {
                    memcpy(m_MessageBuffer, pIPPacket + HeaderLen, m_MessageLen);
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
                        m_Logger->Log->debug("PFRingMarketGateWay::Run Ticker:{} Tick:{}", Ticker, m_MarketData.Tick);
                    }
                }
                pHead->tp_status = TP_STATUS_KERNEL;
            }
        }
    }

protected:
    // 初始化Ring Buffer
    bool InitRingBuffer(int sockfd)
    {
        bool ret = false;
        m_PacketRequest.tp_block_size = getpagesize();
        m_PacketRequest.tp_block_nr = 1000;
        m_PacketRequest.tp_frame_size = 512;
        m_PacketRequest.tp_frame_nr = (m_PacketRequest.tp_block_size / m_PacketRequest.tp_frame_size * m_PacketRequest.tp_block_nr);
        if(setsockopt(sockfd, SOL_PACKET, PACKET_RX_RING, &m_PacketRequest, sizeof(m_PacketRequest)) >= 0)
        {
            m_MMAPAddr = (char *)mmap(0, m_PacketRequest.tp_block_size * m_PacketRequest.tp_block_nr, 
                                        PROT_READ|PROT_WRITE, MAP_SHARED, sockfd, 0);
            if (MAP_FAILED != m_MMAPAddr)
            {
                ret = true;
            }
        }
        m_Logger->Log->info("PFRingMarketGateWay::InitRingBuffer Socket:{} ret:{}", sockfd, ret);
        return ret;
    }
protected:
    char* m_MMAPAddr; // mmap首地址
    struct tpacket_req m_PacketRequest; // 环形缓冲区属性
};


#endif // PFRINGMARKETGATEWAY_HPP