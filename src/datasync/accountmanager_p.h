#ifndef ACCOUNTMANAGER_P_H
#define ACCOUNTMANAGER_P_H

#include <QtCore/QPointer>

#include "qtdatasync_global.h"
#include "exchangeengine_p.h"

#include "rep_accountmanager_p_source.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT AccountManagerPrivate : public AccountManagerPrivateSource
{
	Q_OBJECT

public:
	explicit AccountManagerPrivate(ExchangeEngine *engineParent);

	QString deviceName() const override;
	QByteArray deviceFingerprint() const override;
	void setDeviceName(QString deviceName) override;

public Q_SLOTS:
	void listDevices() override;
	void removeDevice(const QUuid &deviceId) override;
	void updateDeviceKey() override;
	void updateExchangeKey() override;
	void resetAccount(bool keepData) override;
	void exportAccount(quint32 id, bool includeServer) override;
	void exportAccountTrusted(quint32 id, bool includeServer, const QString &password) override;
	void importAccount(quint32 id, const QJsonObject &importData, bool keepData) override;
	void replyToLogin(const QUuid &deviceId, bool accept) override;

private:
	QPointer<ExchangeEngine> _engine;
	Logger *_logger;

	QJsonObject createExportData(bool trusted, bool includeServer);
};

}

#endif // ACCOUNTMANAGER_P_H
