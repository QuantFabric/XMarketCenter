#ifndef PACKETCAPTURE_HPP
#define PACKETCAPTURE_HPP

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

#define LINE_LEN 16
#define INFO_HEX_SPACE 6
#define INFO_BEGIN_POS (LINE_LEN * 3 + INFO_HEX_SPACE)

class PacketCapture
{
public:
    PacketCapture()
    {
        m_Socket = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
        // m_Socket = socket(PF_PACKET, SOCK_DGRAM, htons(ETH_P_IP));
        m_MMAPAddr = NULL;
    }
    // 网卡混杂模式设置
    bool SetPromisc(const char *szIfName, int iFlags)
    {
        bool ret = false;
        ifreq stIfr;
        strcpy(stIfr.ifr_name, szIfName);
        if(ioctl(m_Socket, SIOCGIFFLAGS, &stIfr) >= 0)
        {
            stIfr.ifr_flags = (iFlags) ? (stIfr.ifr_flags | IFF_PROMISC) : (stIfr.ifr_flags & ~IFF_PROMISC);
            if(ioctl(m_Socket, SIOCSIFFLAGS, &stIfr) >= 0)
            {
                ret = true;
            }
        }
        return ret;
    }
    // 网卡绑定
    bool BindDevice(const char * device)
    {
        bool ret = false;
        struct sockaddr_ll sl;
        struct ifreq ifr;
        memset(&sl, 0, sizeof(sl));
        memset(&ifr, 0, sizeof(ifr));
        sl.sll_family = PF_PACKET;
        sl.sll_protocol = htons(ETH_P_IP);
        strncpy(ifr.ifr_name, device, sizeof(ifr.ifr_name));
        if(ioctl(m_Socket, SIOCGIFINDEX, &ifr) >= 0)
        {
            sl.sll_ifindex = ifr.ifr_ifindex;
            ret = bind(m_Socket, (struct sockaddr *)&sl, sizeof(sl)) == 0;
        }
        return ret;
    }

    // 初始化RingBuffer
    bool InitRingBuffer(const char* szIfName)
    {
        bool ret = true;
    	m_PacketRequest.tp_block_size = getpagesize();
        m_PacketRequest.tp_block_nr = 1000;
        m_PacketRequest.tp_frame_size = getpagesize() / 2;
        m_PacketRequest.tp_frame_nr = (m_PacketRequest.tp_block_size / m_PacketRequest.tp_frame_size * m_PacketRequest.tp_block_nr);
        if(setsockopt(m_Socket, SOL_PACKET, PACKET_RX_RING, &m_PacketRequest, sizeof(m_PacketRequest)) < 0)
        {
            ret = -1;
        }
        m_MMAPAddr = (char *)mmap(0, m_PacketRequest.tp_block_size * m_PacketRequest.tp_block_nr, PROT_READ|PROT_WRITE, MAP_SHARED, m_Socket, 0);
        if (MAP_FAILED == m_MMAPAddr)
        {
            ret = false;
        }
        return ret;
    }
    // 从缓冲区提取数据报文
    void Run()
    {
        while(true)
        {
            for(unsigned int index = 0; index < m_PacketRequest.tp_frame_nr; ++index)
            {		
                tpacket_hdr *pHead = (tpacket_hdr *)(m_MMAPAddr + index * m_PacketRequest.tp_frame_size);
                while (TP_STATUS_KERNEL == pHead->tp_status)
                	usleep(10);
                unsigned char *pPacket = (unsigned char *)pHead + pHead->tp_mac;
                printf("tp_snaplen     : %d\n", pHead->tp_snaplen);
                printf("tp_len         : %d\n", pHead->tp_len);
                printf("tp_mac         : %d\n", pHead->tp_mac);
                printf("tp_net         : %d\n", pHead->tp_net);
                printf("tpacket_hdr    : %d\n", sizeof(tpacket_hdr));
                printf("sockaddr_ll    : %d\n", sizeof(sockaddr_ll));
                ParsePacket(pPacket, 0, pHead->tp_snaplen);
                ShowPacket(pPacket, pHead->tp_snaplen);
                pHead->tp_status = TP_STATUS_KERNEL;
            }
        }
    }
protected:
    // 解析数据包
    int ParsePacket(unsigned char* pData, unsigned int offset, unsigned int len)
    {
        return ParseEth(pData, offset, len);
    }
    // 解析以太网层数据
    int ParseEth(unsigned char* pData, unsigned int offset, unsigned int len)
    {
        int ret = -1;
        ethhdr* p = (ethhdr*)(pData + offset);
        printf("src mac: %02x:%02x:%02x:%02x:%02x:%02x\n", p->h_source[0], p->h_source[1], p->h_source[2], p->h_source[3], p->h_source[4], p->h_source[5]);
        printf("des mac: %02x:%02x:%02x:%02x:%02x:%02x\n", p->h_dest[0], p->h_dest[1], p->h_dest[2], p->h_dest[3], p->h_dest[4], p->h_dest[5]);
        switch(htons(p->h_proto))
        {
            case 0x0806: // ARP
                break;
            case 0x8035: // RARP	
                break;
            case 0x0800: // IPv4
                offset = ETH_HLEN;
                ret = ParseIP(pData, offset, len);
                break;
            case 0x86DD: // IPv6	
                break;
            case 0x8100: // VLAN
                break;
        }
        return	ret;
    }
    // 解析IP层数据
    int ParseIP(unsigned char* pData, unsigned int offset, unsigned int len)
    {
        int ret = -1;
        struct in_addr s;
        iphdr* p = (iphdr*)(pData + offset);
        printf("tos         : %02x\n",p->tos);
        printf("total length: %d(0x%02x)\n",ntohs(p->tot_len),ntohs(p->tot_len));
        printf("id          : %d(0x%02x)\n",ntohs(p->id),ntohs(p->id));
        printf("segment flag: %d(0x%02x)\n",ntohs(p->frag_off),ntohs(p->frag_off));
        printf("ttl         : %02x\n",p->ttl);
        printf("protocol    : %02x\n",p->protocol);
        printf("check       : %d(0x%02x)\n",ntohs(p->check), ntohs(p->check));
        s.s_addr = p->saddr;
        printf("src ip      : %s\n",inet_ntoa(s));
        s.s_addr = p->daddr;
        printf("des ip      : %s\n",inet_ntoa(s));
        switch(p->protocol)
        {
            case 0x06:
                printf("protocol    : TCP\n");
                offset += p->ihl * 4;
                ret = ParseTCP(pData, offset, len);	
                break;	
            case 0x11:	
                printf("protocol    : UDP\n");
                offset += p->ihl * 4;
                ret = ParseUDP(pData, offset, len);	
                break;
            case 0x01:
                printf("protocol    : ICMP\n");		
                break;
        }	
        return ret;
    }
    // 解析传输层UDP数据
    int ParseUDP(unsigned char* pData, unsigned int offset, unsigned int len)
    {
        udphdr* p = (udphdr *)(pData + offset);
        printf("src port    : %d\n", ntohs(p->source));
        printf("des port    : %d\n", ntohs(p->dest));
        offset += sizeof(udphdr);
        return ParseApp(pData, offset, len);
    }
    // 解析传输层TCP数据
    int ParseTCP(unsigned char* pData, unsigned int offset, unsigned int len)
    {
        tcphdr* p = (tcphdr*)(pData + offset);
        printf("src port    : %d\n", ntohs(p->source));
        printf("des port    : %d\n", ntohs(p->dest));
        printf("seq         : %d\n", p->seq);
        printf("ack         : %d\n", p->ack_seq);
        offset += p->doff * 4;
        return ParseApp(pData, offset, len);;
    }
    // 解析应用层数据
    int ParseApp(unsigned char* pData, unsigned int offset, unsigned int len)
    {
        ShowHex(pData+offset, len - offset);
	    return 0;
    }

    // 打印空格，直至信息开始的地方　
    void ShowSpace(int line_count)
    {
        int i = 0;
        if(line_count == LINE_LEN)
        {
            while (i < INFO_HEX_SPACE)
            {
                i++;
                printf( " " );
            }
        }
        else if(line_count < LINE_LEN)
        {
            while (i < (INFO_HEX_SPACE + (LINE_LEN-line_count) * 3)) 
            {
                i++;
                printf( " " );
            }
        }
    }

    // 将数据包用16进制打印
    int ShowPacket( unsigned char *pData, unsigned int len)
    {
        printf("full packet :\n");
        for(unsigned int i = 0; i < len; ++i )
        {
            if( i % LINE_LEN == 0 )
            {
                ShowSpace(LINE_LEN);
                for(unsigned int k = i-LINE_LEN; k < i && i != 0 ;k ++)
                {
                    if( pData[k] >= 0x20 && pData[k] <= 0x7F)
                    {
                        printf( "%c", pData[k]);
                    }
                    else
                    {
                        printf( "." );
                    }
                }
                printf( "\n" );
            }
            printf( "%02X ", pData[i]);
            if(i == len - 1)
            {
                ShowSpace(len % LINE_LEN);
                for(unsigned int k = LINE_LEN * (len/LINE_LEN); k < len ;k ++)
                {
                    if( pData[k] >= 0x20 && pData[k] <= 0x7F)
                    {
                        printf( "%c",pData[k]);
                    }
                    else
                    {
                        printf( "." );
                    }
                }
                printf( "\n" );
            }
        }

        printf("\n\n");

        return 0;
    }

    void ShowHex(unsigned char *pData, unsigned int len)
    {
        printf("appinfo     :\n");
        for(unsigned int i = 0; i < len; i++ )
        {
            if(i % 35 == 0 && i != 0)
                printf("\n");
            printf( "%02X", pData[i]);
        }
        printf("\n\n");
    }
protected:
    int m_Socket; // Packet抓包套接字
    char* m_MMAPAddr; // RingBuffer mmap首地址
    struct tpacket_req m_PacketRequest; // 环形缓冲区属性
};

#endif // PACKETCAPTURE_HPP
