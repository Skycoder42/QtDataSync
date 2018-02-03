#ifndef QTDATASYNC_ACCOUNTMANAGER_P_H
#define QTDATASYNC_ACCOUNTMANAGER_P_H

#include <QtCore/QPointer>

#include "qtdatasync_global.h"
#include "exchangeengine_p.h"

#include "rep_accountmanager_p_source.h"

namespace QtDataSync {

//not exported, because base class is not too
class AccountManagerPrivate : public AccountManagerPrivateSource
{
	Q_OBJECT

public:
	explicit AccountManagerPrivate(ExchangeEngine *engineParent);

	QString deviceName() const override;
	QByteArray deviceFingerprint() const override;
	QString lastError() const override;
	void setDeviceName(QString deviceName) override;

public Q_SLOTS:
	void listDevices() override;
	void removeDevice(const QUuid &deviceId) override;
	void updateExchangeKey() override;
	void resetAccount(bool keepData) override;
	void changeRemote(const RemoteConfig &config, bool keepData) override;
	void exportAccount(const QUuid &id, bool includeServer) override;
	void exportAccountTrusted(const QUuid &id, bool includeServer, const QString &password) override;
	void importAccount(const JsonObject &importData, bool keepData) override;
	void importAccountTrusted(const JsonObject &importData, const QString &password, bool keepData) override;
	void replyToLogin(const QUuid &deviceId, bool accept) override;

private Q_SLOTS:
	void requestLogin(const DeviceInfo &deviceInfo);

private:
	QPointer<ExchangeEngine> _engine;
	Logger *_logger;
	QSet<QUuid> _loginRequests;

	QJsonObject serializeExportData(const ExportData &data) const;
	ExportData deserializeExportData(const QJsonObject &importData) const;
};

}

#endif // QTDATASYNC_ACCOUNTMANAGER_P_H
