#ifndef QTDATASYNC_MIGRATIONHELPER_H
#define QTDATASYNC_MIGRATIONHELPER_H

#include <QtCore/qobject.h>
#include <QtCore/qscopedpointer.h>

#include "QtDataSync/qtdatasync_global.h"

namespace QtDataSync {

class MigrationHelperPrivate;
class Q_DATASYNC_EXPORT MigrationHelper : public QObject
{
	Q_OBJECT

public:
	enum MigrationFlag {
		MigrateData = 0x01,
		MigrateChanged = (0x02 | MigrateData),
		MigrateRemoteConfig = 0x04,
		MigrateAll = (MigrateData | MigrateChanged | MigrateRemoteConfig),

		MigrateWithCleanup = 0x08,
		MigrateOverwriteConfig = 0x10,
		MigrateOverwriteData = 0x20
	};
	Q_DECLARE_FLAGS(MigrationFlags, MigrationFlag)
	Q_FLAG(MigrationFlags)

	static const QString DefaultOldStorageDir;

	explicit MigrationHelper(QObject *parent = nullptr);
	explicit MigrationHelper(const QString &setupName, QObject *parent = nullptr);
	~MigrationHelper();

public Q_SLOTS:
	void startMigration(const QString &storageDir = DefaultOldStorageDir, MigrationFlags flags = MigrationFlags(MigrateAll) | MigrateWithCleanup);

Q_SIGNALS:
	void migrationPrepared(int operations);
	void migrationProgress(int current);
	void migrationDone(bool ok);

private:
	QScopedPointer<MigrationHelperPrivate> d;
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(QtDataSync::MigrationHelper::MigrationFlags)

#endif // QTDATASYNC_MIGRATIONHELPER_H
