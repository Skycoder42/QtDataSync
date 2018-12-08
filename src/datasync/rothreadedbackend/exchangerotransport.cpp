#include "exchangerotransport_p.h"
#include "exchangebuffer_p.h"
#include "exchangebufferserver_p.h"
using namespace QtDataSync;

bool ThreadedQtRoServer::prepareHostNode(const QUrl &url, QRemoteObjectHostBase *host)
{
	auto server = new ExchangeBufferServer{host};
	if(server->registerInstance(url))
		return true;
	else {
		delete server;
		return false;
	}
}

bool ThreadedQtRoClient::prepareClientNode(const QUrl &url, QRemoteObjectNode *node)
{
	auto buffer = new ExchangeBuffer{node};
	QObject::connect(buffer, &ExchangeBuffer::partnerConnected,
					 node, [buffer, node]() {
		node->addClientSideConnection(buffer);
	});

	if(ExchangeBufferServer::connectTo(url, buffer))
		return true;
	else {
		delete buffer;
		return false;
	}
}
