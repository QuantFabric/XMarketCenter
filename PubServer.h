#ifndef PUBSERVER_H
#define PUBSERVER_H
#include "SHMServer.hpp"
#include "PackMessage.hpp"

struct ServerConf : public SHMIPC::CommonConf
{
    static const bool Publish = true;
};

class PubServer: public SHMIPC::SHMServer<Message::PackMessage, ServerConf>
{
public:
    PubServer();

    virtual ~PubServer();

    void HandleMsg();
};


#endif // PUBSERVER_H