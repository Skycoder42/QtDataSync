#ifndef SETUP_P_H
#define SETUP_P_H

#include "setup.h"
#include <QJsonSerializer>
#include <QMutex>

namespace QtDataSync {

class SetupPrivate
{
public:
	static QMutex setupMutex;
	static QHash<QString, Setup*> setups;

	QJsonSerializer *serializer;

	SetupPrivate(Setup *parent);
};

}

#endif // SETUP_P_H
