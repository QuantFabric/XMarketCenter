#include "PubServer.h"


PubServer::PubServer():SHMServer<Message::PackMessage, ServerConf>()
{

}

PubServer::~PubServer()
{

}

void PubServer::HandleMsg()
{

}