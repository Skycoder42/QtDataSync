#include "extensions.h"

DummyAuth::DummyAuth(int value, QObject *parent) :
	IAuthenticator{parent},
	value{value}
{}

void DummyAuth::signIn() {}

void DummyAuth::logOut() {}

void DummyAuth::abortRequest() {}



DummyTransform::DummyTransform(int value, QObject *parent) :
	ICloudTransformer{parent},
	value{value}
{}

void DummyTransform::transformUpload(const QtDataSync::LocalData &) {}

void DummyTransform::transformDownload(const QtDataSync::CloudData &) {}
