#ifndef QTDATASYNC_QTROTRANSPORTREGISTRY_H
#define QTDATASYNC_QTROTRANSPORTREGISTRY_H

#include <type_traits>
#include <functional>

#include <QtCore/qurl.h>
#include <QtRemoteObjects/qremoteobjectnode.h>

#include "QtDataSync/qtdatasync_global.h"

namespace QtDataSync {

namespace QtRoTransportRegistry
{

class Q_DATASYNC_EXPORT Server
{
public:
	virtual ~Server();

	virtual bool prepareHostNode(const QUrl &url, QRemoteObjectHostBase *host) = 0;
};

class Q_DATASYNC_EXPORT Client
{
public:
	virtual ~Client();

	virtual bool prepareClientNode(const QUrl &url, QRemoteObjectNode *node) = 0;
};

Q_DATASYNC_EXPORT void registerTransport(const QString &urlScheme,
										 Server *server,
										 Client *client);
template <typename TFuncServer, typename TFuncClient>
void registerTransportFunctions(const QString &urlScheme, TFuncServer &&server, TFuncClient &&client);

Q_DATASYNC_EXPORT bool connectHostBaseNode(const QUrl &url, QRemoteObjectHostBase *node);
Q_DATASYNC_EXPORT void connectHostNode(const QUrl &url, QRemoteObjectHost *node);
Q_DATASYNC_EXPORT bool connectClientNode(const QUrl &url, QRemoteObjectNode *node);

// ------------- GENERIC IMPLEMENTATION -------------

namespace __private {

class Q_DATASYNC_EXPORT FuncServer : public Server
{
public:
	FuncServer(std::function<bool(QUrl,QRemoteObjectHostBase*)> &&fn);

	bool prepareHostNode(const QUrl &url, QRemoteObjectHostBase *host) override;

private:
	std::function<bool(QUrl,QRemoteObjectHostBase*)> _fn;
};

class Q_DATASYNC_EXPORT FuncClient : public Client
{
public:
	FuncClient(std::function<bool(QUrl,QRemoteObjectNode*)> &&fn);

	bool prepareClientNode(const QUrl &url, QRemoteObjectNode *node) override;

private:
	std::function<bool(QUrl,QRemoteObjectNode*)> _fn;
};

}

template <typename TFuncServer, typename TFuncClient>
void registerTransportFunctions(const QString &urlScheme, TFuncServer &&server, TFuncClient &&client)
{
	registerTransport(urlScheme,
					  new __private::FuncServer{std::forward<TFuncServer>(server)},
					  new __private::FuncClient{std::forward<TFuncClient>(client)});
}

}

}

#endif // QTDATASYNC_QTROTRANSPORTREGISTRY_H
