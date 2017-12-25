#ifndef DATABASECONTROLLER_H
#define DATABASECONTROLLER_H

#include <tuple>

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QThreadPool>
#include <QtCore/QThreadStorage>
#include <QtCore/QUuid>
#include <QtCore/QJsonObject>
#include <QtCore/QException>
#include <QtCore/QTimer>

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlDriver>

#include "asymmetriccrypto_p.h"

class DatabaseException : public QException
{
public:
	DatabaseException(const QSqlError &error);
	DatabaseException(QSqlDatabase db);
	DatabaseException(const QSqlQuery &query);

	QSqlError error() const;

	const char *what() const noexcept override;
	void raise() const override;
	QException *clone() const override;

private:
	QSqlError _error;
	const QByteArray _msg;
};

class DatabaseController : public QObject
{
	Q_OBJECT

public:
	explicit DatabaseController(QObject *parent = nullptr);

	void cleanupDevices(quint64 offlineSinceDays);

	QUuid addNewDevice(const QString &name,
					   const QByteArray &signScheme,
					   const QByteArray &signKey,
					   const QByteArray &cryptScheme,
					   const QByteArray &cryptKey);
	QtDataSync::AsymmetricCryptoInfo *loadCrypto(const QUuid &deviceId,
																	  CryptoPP::RandomNumberGenerator &rng,
																	  QObject *parent = nullptr);
	void updateLogin(const QUuid &deviceId, const QString &name);

	void addChange(const QUuid &deviceId,
				   const QByteArray &dataId,
				   const quint32 keyIndex,
				   const QByteArray &salt,
				   const QByteArray &data);

	quint64 changeCount(const QUuid &deviceId);
	QList<std::tuple<quint64, quint32, QByteArray, QByteArray>> loadNextChanges(const QUuid &deviceId, quint32 count, quint32 skip);
	void completeChange(const QUuid &deviceId, quint64 dataIndex);

Q_SIGNALS:
	void notifyChanged(const QUuid &deviceId);

	void databaseInitDone(bool success);
	void cleanupOperationDone(int rowsAffected, const QString &error = {});

private Q_SLOTS:
	void onNotify(const QString &name, QSqlDriver::NotificationSource source, const QVariant &payload);
	void timeout();

private:
	class DatabaseWrapper
	{
	public:
		DatabaseWrapper();
		~DatabaseWrapper();

		QSqlDatabase database() const;
	private:
		QString dbName;
	};

	QThreadStorage<DatabaseWrapper> _threadStore;
	QTimer *_keepAliveTimer;

	void initDatabase();
};

#endif // DATABASECONTROLLER_H
