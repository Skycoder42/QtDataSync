#ifndef REMOTECONNECTOR_H
#define REMOTECONNECTOR_H

#include "qtdatasync_global.h"
#include "stateholder.h"
#include <QJsonValue>
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
	virtual void download(const ObjectKey &key, const QByteArray &keyProperty) = 0;
	virtual void upload(const ObjectKey &key, const QJsonObject &object, const QByteArray &keyProperty) = 0;//auto unchanged
	virtual void remove(const ObjectKey &key, const QByteArray &keyProperty) = 0;//auto unchanged
	virtual void markUnchanged(const ObjectKey &key, const QByteArray &keyProperty) = 0;

signals:
	void remoteStateChanged(bool canUpdate, const StateHolder::ChangeHash &remoteChanges);

	void operationDone(const QJsonValue &result = QJsonValue::Undefined);
	void operationFailed(const QString &error);

protected:
	virtual QByteArray loadDeviceId();
	virtual void resetDeviceId();
};

}

#endif // REMOTECONNECTOR_H
