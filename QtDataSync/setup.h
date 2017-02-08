#ifndef SETUP_H
#define SETUP_H

#include "qtdatasync_global.h"
#include <QObject>
class QJsonSerializer;

namespace QtDataSync {

class LocalStore;

class SetupPrivate;
class Setup
{
	Q_DISABLE_COPY(Setup)
public:
	static const QString DefaultSetup;

	static void setCleanupTimeout(unsigned long timeout);
	static void removeSetup(const QString &name);

	Setup();
	~Setup();

	QJsonSerializer *serializer() const;
	LocalStore *localStore() const;

	Setup &setSerializer(QJsonSerializer *serializer);
	Setup &setLocalStore(LocalStore *localStore);
	void create(const QString &name = DefaultSetup);

private:
	QScopedPointer<SetupPrivate> d;
};

}

#endif // SETUP_H
