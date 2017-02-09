#include "authenticator.h"
using namespace QtDataSync;

Authenticator::Authenticator(RemoteConnector *connector, QObject *parent) :
	QObject(parent),
	_connector(connector)
{}
