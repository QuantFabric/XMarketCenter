

#include <time.h>
#include <sched.h>
#include <vector>
#include <string>
#include <thread>
#include <stdio.h>
#include "Common.hpp"
#include "SHMConnection.hpp"
#include "PackMessage.hpp"

struct ClientConf : public SHMIPC::CommonConf
{
    static const bool Publish = true;
    static const bool Performance = true;
};

int main(int argc, char** argv) 
{
    if(argc < 3)
    {
        printf("Usage: %s <ClientName> <ServerName>\n", argv[0]);
        return -1;
    }

    SHMIPC::SHMConnection<Message::PackMessage, ClientConf> client(argv[1]);
    client.Start(argv[2]);
    Message::PackMessage recvMsg;
    static uint64_t i = 0;
    while(true)
    {
        if(client.Pop(recvMsg))
        {
            i++;
            printf("recv msg %s %s\n", recvMsg.FutureMarketData.Ticker, recvMsg.FutureMarketData.RecvLocalTime);
        }
    }
    
    return 0;
}
// g++ -std=c++17 -O3 -o MarketReader MarketReader.cpp -lpthread -lrt