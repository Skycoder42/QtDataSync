#ifndef SETUP_H
#define SETUP_H

#include "qtdatasync_global.h"
#include <QObject>
class QJsonSerializer;

namespace QtDataSync {

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

	Setup &setSerializer(QJsonSerializer *serializer);
	void create(const QString &name = DefaultSetup);

private:
	QScopedPointer<SetupPrivate> d;
};

}

#endif // SETUP_H
