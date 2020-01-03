#ifndef ANONAUTH_H
#define ANONAUTH_H

#include <QtDataSync/FirebaseAuthenticator>

class AnonAuth : public QtDataSync::FirebaseAuthenticator
{
	Q_OBJECT

public:
	explicit AnonAuth(QtDataSync::Engine *engine = nullptr);

public Q_SLOTS:
	void abortSignIn() override;

protected:
	void firebaseSignIn() override;
};

#endif // ANONAUTH_H
