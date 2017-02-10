#include "authenticator.h"
#include "remoteconnector.h"
using namespace QtDataSync;

Authenticator::Authenticator(QObject *parent) :
	QObject(parent)
{}

void Authenticator::resetDeviceId()
{
	QMetaObject::invokeMethod(connector(), "resetDeviceId");
}
