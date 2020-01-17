#ifndef ANONAUTH_H
#define ANONAUTH_H

#include <QtCore/QPointer>

#include <QtNetwork/QNetworkReply>

#include <QtDataSync/IAuthenticator>

class AnonAuth : public QtDataSync::IAuthenticator
{
	Q_OBJECT

public:
	explicit AnonAuth(QObject *parent = nullptr);

	void forceExpire(const QDateTime &when);

	void signIn() override;
	void logOut() override;
	void abortRequest() override;

private:
	QPointer<QNetworkReply> _authReply;
};

#endif // ANONAUTH_H
