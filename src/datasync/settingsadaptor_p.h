#ifndef QTDATASYNC_SETTINGSADAPTOR_P_H
#define QTDATASYNC_SETTINGSADAPTOR_P_H

#include "qtdatasync_global.h"
#include "tablesynccontroller.h"

#include <QtCore/QObject>
#include <QtCore/QSettings>
#include <QtCore/QVersionNumber>
#include <QtCore/QDataStream>
#include <QtCore/QLoggingCategory>
#include <QtCore/QTemporaryFile>

#include <QtSql/QSqlDatabase>

namespace QtDataSync {

struct SettingsInfo {
	QString connectionName;
	QString tableName;
	quint64 rnd;
};

class Q_DATASYNC_EXPORT SettingsAdaptor : public QObject
{
	Q_OBJECT

public:
	static const QString SettingsInfoConnectionKey;
	static const QString SettingsInfoTableKey;

	static QSettings::Format registerFormat();
	static bool createTable(QSqlDatabase database, const QString &table);

	explicit SettingsAdaptor(Engine *engine,
							 QString table,
							 QSqlDatabase database);

	QString path() const;

private Q_SLOTS:
	void syncStateChanged(TableSyncController::SyncState state);
	void tableEvent(const QString &name);

private:
	static bool readSettings(QIODevice &device, QSettings::SettingsMap &map);
	static bool writeSettings(QIODevice &device, const QSettings::SettingsMap &map);

	static bool loadValue(const QSettings::SettingsMap &map, const QString &key, QString &target);
	static bool updateFile(QIODevice *device, SettingsInfo info, const QString &devName);

	SettingsInfo _info;
	QTemporaryFile *_syncFile;
	TableSyncController *_controller;
	void updateSettings();
};

Q_DECLARE_LOGGING_CATEGORY(logSettings)

}

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const QtDataSync::SettingsInfo &settings);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, QtDataSync::SettingsInfo &settings);

#endif // QTDATASYNC_SETTINGSADAPTOR_P_H
