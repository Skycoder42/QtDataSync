#ifndef QTDATASYNC_MIGRATIONHELPER_H
#define QTDATASYNC_MIGRATIONHELPER_H

#include <QtCore/qobject.h>
#include <QtCore/qscopedpointer.h>

#include "QtDataSync/qtdatasync_global.h"

namespace QtDataSync {

class MigrationHelperPrivate;
//! A helper class to migrate data from DataSync 3.0 to this version
class Q_DATASYNC_EXPORT MigrationHelper : public QObject
{
	Q_OBJECT

public:
	//! Flags to configure what and how should be migrated
	enum MigrationFlag {
		MigrateData = 0x01, //!< Migrate actual stored data
		MigrateChanged = (0x02 | MigrateData), //!< Migrate the changed state of that data
		MigrateRemoteConfig = 0x04, //!< Migrate the connection configuration

		MigrateWithCleanup = 0x08, //!< Remove all old data after a successful migration
		MigrateOverwriteConfig = 0x10, //!< Overwrite existing configurations with the migrated remote config
		MigrateOverwriteData = 0x20, //!< Overwrite existing data with the migrated data

		MigrateDefault = (MigrateData | MigrateRemoteConfig | MigrateWithCleanup) //!< The default flags when leaving out the parameter
	};
	Q_DECLARE_FLAGS(MigrationFlags, MigrationFlag)
	Q_FLAG(MigrationFlags)

	//! The previous default path where data was stored
	static const QString DefaultOldStorageDir;

	//! Default constructor, uses the default setup
	explicit MigrationHelper(QObject *parent = nullptr);
	//! Constructor with an explicit setup
	explicit MigrationHelper(const QString &setupName, QObject *parent = nullptr);
	~MigrationHelper();

public Q_SLOTS:
	//! Start the migration process from the given directory with flags
	void startMigration(const QString &storageDir = DefaultOldStorageDir, MigrationFlags flags = MigrateDefault);

Q_SIGNALS:
	//! Is emitted when the number of migrate operations has been determined
	void migrationPrepared(int operations);
	//! Is emitted for every successful migration step done to give a progress
	void migrationProgress(int current);
	//! Is emitted when the migration has been finished
	void migrationDone(bool ok);

private:
	QScopedPointer<MigrationHelperPrivate> d;
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(QtDataSync::MigrationHelper::MigrationFlags)

#endif // QTDATASYNC_MIGRATIONHELPER_H
