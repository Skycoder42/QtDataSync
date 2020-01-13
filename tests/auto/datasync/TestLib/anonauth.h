#ifndef ANONAUTH_H
#define ANONAUTH_H

#include <QtCore/QPointer>

#include <QtNetwork/QNetworkReply>

#include <QtDataSync/FirebaseAuthenticator>

class AnonAuth : public QtDataSync::FirebaseAuthenticator
{
	Q_OBJECT

public:
	explicit AnonAuth(const QString &apiKey, QSettings *settings, QObject *parent = nullptr);

public Q_SLOTS:
	void abortSignIn() override;

protected:
	void firebaseSignIn() override;

private:
	QPointer<QNetworkReply> _authReply;
};

#endif // ANONAUTH_H
