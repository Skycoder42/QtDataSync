#ifndef FAKEAUTH_H
#define FAKEAUTH_H

#include <QtDataSync/IAuthenticator>

class FakeAuth : public QtDataSync::IAuthenticator
{
	Q_OBJECT

public:
	explicit FakeAuth(QObject *parent = nullptr);

protected:
	void signIn() override;
	void logOut() override;
	void abortRequest() override;
};

#endif // FAKEAUTH_H
