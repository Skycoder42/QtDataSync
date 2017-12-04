#include "remoteconnector_p.h"
using namespace QtDataSync;

RemoteConnector::RemoteConnector(const Defaults &defaults, QObject *parent) :
	QObject(parent),
	_defaults(defaults),
	_logger(_defaults.createLogger("connector", this)),
	_settings(_defaults.createSettings(this, QStringLiteral("connector")))
{}
