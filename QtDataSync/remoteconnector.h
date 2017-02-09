#ifndef REMOTECONNECTOR_H
#define REMOTECONNECTOR_H

#include "qtdatasync_global.h"
#include "stateholder.h"
#include <QObject>

namespace QtDataSync {

class Authenticator;

class QTDATASYNCSHARED_EXPORT RemoteConnector : public QObject
{
	Q_OBJECT

public:
	explicit RemoteConnector(QObject *parent = nullptr);

	virtual void initialize();
	virtual void finalize();

	virtual Authenticator *createAuthenticator(QObject *parent) = 0;

public slots:
	virtual void download(const QtDataSync::StateHolder::ChangeKey &key, const QString &value) = 0;
	virtual void upload(const QtDataSync::StateHolder::ChangeKey &key, const QString &value, const QJsonObject &object) = 0;
	virtual void remove(const QtDataSync::StateHolder::ChangeKey &key, const QString &value) = 0;
	virtual void markUnchanged(const QtDataSync::StateHolder::ChangeKey &key, const QString &value) = 0;

signals:
	void remoteStateChanged(bool canUpdate, const QtDataSync::StateHolder::ChangeHash &remoteChanges);
	void operationDone(const QtDataSync::StateHolder::ChangeKey &key, const QString &value, const QJsonValue &result);
	void operationFailed(const QtDataSync::StateHolder::ChangeKey &key, const QString &value, const QString &error);

protected:
	virtual QByteArray loadDeviceId();
	virtual void resetDeviceId();
};

}

#endif // REMOTECONNECTOR_H
