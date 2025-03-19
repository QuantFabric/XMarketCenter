#ifndef PUBSERVER_HPP
#define PUBSERVER_HPP
#include "SHMServer.hpp"
#include "PackMessage.hpp"

struct ServerConf : public SHMIPC::CommonConf
{
    static const bool Publish = true;
    static const bool Performance = false;
};

class PubServer: public SHMIPC::SHMServer<Message::PackMessage, ServerConf>
{
public:
    PubServer():SHMServer<Message::PackMessage, ServerConf>()
    {

    }

    virtual ~PubServer()
    {

    }

    void HandleMsg()
    {

    }
};


#endif // PUBSERVER_HPP