#ifndef DATABASECONTROLLER_H
#define DATABASECONTROLLER_H

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QThreadPool>
#include <QtCore/QThreadStorage>
#include <QtCore/QUuid>
#include <QtCore/QJsonObject>
#include <QtCore/QException>

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>

#include "asymmetriccrypto_p.h"

class DatabaseException : public QException
{
public:
	DatabaseException(const QSqlError &error);
	DatabaseException(QSqlDatabase db);
	DatabaseException(const QSqlQuery &query, QSqlDatabase db = {});

	QSqlError error() const;
	QString errorString() const;

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
												 QString &name,
												 QObject *parent = nullptr);
	void updateName(const QUuid &deviceId, const QString &name);

signals:
	//TODO correct
	void notifyChanged(const QUuid &userId,
					   const QUuid &excludedDeviceId,
					   const QString &type,
					   const QString &key,
					   bool changed);

	void databaseInitDone(bool success);
	void cleanupOperationDone(int rowsAffected, const QString &error = {});

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

	bool multiThreaded;
	QThreadStorage<DatabaseWrapper> threadStore;

	void initDatabase();
};

#endif // DATABASECONTROLLER_H
