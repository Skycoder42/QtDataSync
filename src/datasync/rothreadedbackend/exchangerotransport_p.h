#ifndef THREADEDCLIENT_P_H
#define THREADEDCLIENT_P_H

#include "qtdatasync_global.h"
#include "qtrotransportregistry.h"

namespace QtDataSync {

class ThreadedQtRoServer : public QtRoTransportRegistry::Server
{
public:
	bool prepareHostNode(const QUrl &url, QRemoteObjectHostBase *host) override;
};

class ThreadedQtRoClient : public QtRoTransportRegistry::Client
{
public:
	bool prepareClientNode(const QUrl &url, QRemoteObjectNode *node) override;
};

}

#endif // THREADEDCLIENT_P_H
