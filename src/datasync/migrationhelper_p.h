#ifndef QTDATASYNC_MIGRATIONHELPER_P_H
#define QTDATASYNC_MIGRATIONHELPER_P_H

#include <QtCore/QRunnable>
#include <QtCore/QPointer>

#include "qtdatasync_global.h"
#include "migrationhelper.h"
#include "defaults.h"

namespace QtDataSync {

class MigrationRunnable : public QRunnable
{
public:
	MigrationRunnable(const Defaults &defaults,
					  MigrationHelper *helper,
					  const QString &oldDir,
					  MigrationHelper::MigrationFlags flags);

	void run() override;

private:
	Defaults _defaults;
	const QPointer<MigrationHelper> _helper;
	const QString _oldDir;
	const MigrationHelper::MigrationFlags _flags;

	QPointer<Logger> _logger;
	int _progress;

	void migrationPrepared(int count);
	void migrationProgress();
	void migrationDone(bool ok);

	void copyConf(QSettings *old, const QString &oldKey, QSettings *current, const QString &newKey) const;
};

class MigrationHelperPrivate
{
public:
	MigrationHelperPrivate(const QString &setupName);

	Defaults defaults;
};

}

#endif // QTDATASYNC_MIGRATIONHELPER_P_H
