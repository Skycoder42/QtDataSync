#ifndef REMOTECONNECTOR_H
#define REMOTECONNECTOR_H

#include "defaults.h"
#include "qtdatasync_global.h"
#include "stateholder.h"
#include <QJsonValue>
#include <QJsonObject>
#include <QObject>

namespace QtDataSync {

class Authenticator;

class QTDATASYNCSHARED_EXPORT RemoteConnector : public QObject
{
	Q_OBJECT

public:
	explicit RemoteConnector(QObject *parent = nullptr);

	virtual void initialize(Defaults *defaults);
	virtual void finalize();

	virtual Authenticator *createAuthenticator(Defaults *defaults, QObject *parent) = 0;

public slots:
	virtual void download(const ObjectKey &key, const QByteArray &keyProperty) = 0;
	virtual void upload(const ObjectKey &key, const QJsonObject &object, const QByteArray &keyProperty) = 0;//auto unchanged
	virtual void remove(const ObjectKey &key, const QByteArray &keyProperty) = 0;//auto unchanged
	virtual void markUnchanged(const ObjectKey &key, const QByteArray &keyProperty) = 0;

	virtual void resetDeviceId();

signals:
	void remoteStateChanged(bool canUpdate, const StateHolder::ChangeHash &remoteChanges);

	void operationDone(const QJsonValue &result = QJsonValue::Undefined);
	void operationFailed(const QString &error);

protected:
	Defaults *defaults() const;
	virtual QByteArray loadDeviceId();

private:
	Defaults *_defaults;
};

}

#endif // REMOTECONNECTOR_H
