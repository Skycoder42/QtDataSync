#ifndef MOCKREMOTECONNECTOR_H
#define MOCKREMOTECONNECTOR_H

#include <QObject>
#include <remoteconnector.h>

class MockRemoteConnector : public QtDataSync::RemoteConnector
{
	Q_OBJECT

public:
	explicit MockRemoteConnector(QObject *parent = nullptr);

	QtDataSync::Authenticator *createAuthenticator(QtDataSync::Defaults *defaults, QObject *parent) override;

	//WARNING!!! mutex must be unlocked
	void updateConnected(bool resync);
	void doChangeEmit();

public slots:
	void reloadRemoteState() override;
	void requestResync() override;
	void download(const QtDataSync::ObjectKey &key, const QByteArray &keyProperty) override;
	void upload(const QtDataSync::ObjectKey &key, const QJsonObject &object, const QByteArray &keyProperty) override;
	void remove(const QtDataSync::ObjectKey &key, const QByteArray &keyProperty) override;
	void markUnchanged(const QtDataSync::ObjectKey &key, const QByteArray &keyProperty) override;

protected:
	void resetUserData(const QVariant &extraData) override;

public:
	QMutex mutex;
	bool enabled;
	bool connected;
	int failCount;
	int reloadTimeout;
	QHash<QtDataSync::ObjectKey, QJsonObject> pseudoStore;
	QtDataSync::StateHolder::ChangeHash pseudoState;
	QList<QtDataSync::ObjectKey> emitList;

	Q_INVOKABLE void doChangeEmitImpl();
};

#endif // MOCKREMOTECONNECTOR_H
