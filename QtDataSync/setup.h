#ifndef SETUP_H
#define SETUP_H

#include "qtdatasync_global.h"
#include <QObject>
class QJsonSerializer;

namespace QtDataSync {

class SetupPrivate;
class Setup : public QObject
{
	Q_OBJECT

public:
	static const QString DefaultSetup;

	static Setup *createSetup(const QString &name = DefaultSetup);
	static Setup *setup(const QString &name = DefaultSetup);
	static void deleteSetup(const QString &name = DefaultSetup);

	QJsonSerializer *serializer() const;
	void setSerializer(QJsonSerializer *serializer);

private:
	QScopedPointer<SetupPrivate> d;

	explicit Setup();
	~Setup();
};

}

#endif // SETUP_H
