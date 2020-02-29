#ifndef ANONAUTH_H
#define ANONAUTH_H

#include <chrono>
#include <optional>

#include <QtCore/QPointer>

#include <QtNetwork/QNetworkReply>

#include <QtDataSync/IAuthenticator>

class AnonAuth : public QtDataSync::IAuthenticator
{
	Q_OBJECT

public:
	explicit AnonAuth(QObject *parent = nullptr);

	void setOverwriteExpire(std::optional<std::chrono::seconds> delta);

	bool block = false;
	QString localId;
	QString idToken;

protected:
	void signIn() override;
	void logOut() override;
	void abortRequest() override;

private:
	std::optional<std::chrono::seconds> _expireOw = std::nullopt;
	QPointer<QNetworkReply> _authReply;
};

#endif // ANONAUTH_H
