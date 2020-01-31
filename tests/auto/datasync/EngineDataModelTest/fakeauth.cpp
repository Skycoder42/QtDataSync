#include "fakeauth.h"

FakeAuth::FakeAuth(QObject *parent) :
	IAuthenticator{parent}
{}

void FakeAuth::signIn() {}

void FakeAuth::logOut() {}

void FakeAuth::abortRequest() {}
