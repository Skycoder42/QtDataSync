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

	void initialize();

	void cleanupDevices();

	QUuid addNewDevice(const QString &name,
					   const QByteArray &signScheme,
					   const QByteArray &signKey,
					   const QByteArray &cryptScheme,
					   const QByteArray &cryptKey,
					   const QByteArray &fingerprint,
					   const QByteArray &keyCmac);
	void addNewDeviceToUser(const QUuid &newDeviceId,
							const QUuid &partnerDeviceId,
							const QString &name,
							const QByteArray &signScheme,
							const QByteArray &signKey,
							const QByteArray &cryptScheme,
							const QByteArray &cryptKey,
							const QByteArray &fingerprint);
	QtDataSync::AsymmetricCryptoInfo *loadCrypto(const QUuid &deviceId,
												 CryptoPP::RandomNumberGenerator &rng,
												 QObject *parent = nullptr);
	void updateLogin(const QUuid &deviceId, const QString &name);
	bool updateCmac(const QUuid &deviceId, quint32 keyIndex, const QByteArray &cmac);
	QList<std::tuple<QUuid, QString, QByteArray>> listDevices(const QUuid &deviceId); // (deviceid, name, fingerprint)
	void removeDevice(const QUuid &deviceId, const QUuid &deleteId);

	bool addChange(const QUuid &deviceId,
				   const QByteArray &dataId,
				   const quint32 keyIndex,
				   const QByteArray &salt,
				   const QByteArray &data);
	bool addDeviceChange(const QUuid &deviceId,
						 const QUuid &targetId,
						 const QByteArray &dataId,
						 const quint32 keyIndex,
						 const QByteArray &salt,
						 const QByteArray &data);

	quint32 changeCount(const QUuid &deviceId);
	QList<std::tuple<quint64, quint32, QByteArray, QByteArray>> loadNextChanges(const QUuid &deviceId, quint32 count, quint32 skip); // (dataid, keyindex, salt, data)
	void completeChange(const QUuid &deviceId, quint64 dataIndex);

	QList<std::tuple<QUuid, QByteArray, QByteArray, QByteArray>> tryKeyChange(const QUuid &deviceId, quint32 proposedIndex, int &offset); //(deviceid, scheme, key, cmac)
	bool updateExchangeKey(const QUuid &deviceId,
						   quint32 keyIndex,
						   const QByteArray &scheme, const QByteArray &cmac,
						   const QList<std::tuple<QUuid, QByteArray, QByteArray>> &deviceKeys);// (deviceId, key, cmac)
	std::tuple<quint32, QByteArray, QByteArray, QByteArray> loadKeyChanges(const QUuid &deviceId);// (keyIndex, scheme, key, cmac)

Q_SIGNALS:
	void notifyChanged(const QUuid &deviceId);

	void databaseInitDone(bool success);

private Q_SLOTS:
	void dbInitDone(bool success);
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

	static QThreadStorage<DatabaseWrapper> _threadStore; //must be static
	QTimer *_keepAliveTimer;
	QTimer *_cleanupTimer;

	void initDatabase(quint64 quota, bool forceQuota);
	void updateQuotaLimit(quint64 quota, bool forceQuota);
};

#endif // DATABASECONTROLLER_H
