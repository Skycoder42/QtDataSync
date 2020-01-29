#ifndef QTDATASYNC_SETTINGSADAPTOR_P_H
#define QTDATASYNC_SETTINGSADAPTOR_P_H

#include "qtdatasync_global.h"

#include <QtCore/QObject>
#include <QtCore/QSettings>
#include <QtCore/QVersionNumber>
#include <QtCore/QDataStream>
#include <QtCore/QLoggingCategory>

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
	static QSettings::Format registerFormat();
	static void createTable(QSqlDatabase database, const QString &name);  // throws SqlException

	explicit SettingsAdaptor(QSettings *settings);

private:
	static bool readSettings(QIODevice &device, QSettings::SettingsMap &map);
	static bool writeSettings(QIODevice &device, const QSettings::SettingsMap &map);

	QSettings *settings() const;
};

Q_DECLARE_LOGGING_CATEGORY(logSettings)

}

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const QtDataSync::SettingsInfo &settings);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, QtDataSync::SettingsInfo &settings);

#endif // QTDATASYNC_SETTINGSADAPTOR_P_H
