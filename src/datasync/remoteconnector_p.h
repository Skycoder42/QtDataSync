#ifndef QTDATASYNC_REMOTECONNECTOR_H
#define QTDATASYNC_REMOTECONNECTOR_H

#include "qtdatasync_global.h"
#include "engine.h"
#include "objectkey.h"
#include "icloudtransformer.h"

#include <QtCore/QObject>
#include <QtCore/QLoggingCategory>

#include "realtimedb_api.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT RemoteConnector : public QObject
{
	Q_OBJECT

public:
	static constexpr int CodeNotFound = 404;
	static constexpr int CodeETagMismatch = 412;
	static const QByteArray NullETag;

	explicit RemoteConnector(const QString &userId, Engine *engine);

	void setIdToken(const QString &idToken);

public Q_SLOTS:
	void startLiveSync();
	void unblockLiveSync();
	void stopLiveSync();

	void getChanges(const QByteArray &type, const QDateTime &since);
	void uploadChange(const CloudData &data);
	void removeTable(const QByteArray &type);

Q_SIGNALS:
	void triggerSync(QPrivateSignal = {});

	void downloadedData(const CloudData &data, QPrivateSignal = {});
	void updateLastSync(const QDateTime &lastSync, QPrivateSignal = {});
	void syncDone(const QByteArray &type, QPrivateSignal = {});
	void uploadedData(const ObjectKey &key, QPrivateSignal = {});
	void removedTable(const QByteArray &type, QPrivateSignal = {});

	void networkError(const QString &error, QPrivateSignal = {});

private Q_SLOTS:
	void apiError(const QString &errorString, int errorCode, QtRestClient::RestReply::Error errorType);

	void streamEvent();
	void streamClosed();

private:
	firebase::realtimedb::ApiClient *_api = nullptr;
	int _limit = 100;

	QNetworkReply *_eventStream = nullptr;
	bool _liveSyncBlocked = true;

	static QString translateError(const QtDataSync::firebase::realtimedb::Error &error, int code);

	QString timeString(const std::chrono::milliseconds &time);
	void doUpload(const CloudData &data, QByteArray eTag);
};

class AccurateTimestampConverter : public QtJsonSerializer::TypeConverter
{
public:
	AccurateTimestampConverter();
	QT_JSONSERIALIZER_TYPECONVERTER_NAME(AccurateTimestampConverter)
	bool canConvert(int metaTypeId) const override;
	QList<QCborValue::Type> allowedCborTypes(int metaTypeId, QCborTag tag) const override;
	QCborValue serialize(int propertyType, const QVariant &value) const override;
	QVariant deserializeCbor(int propertyType, const QCborValue &value, QObject *parent) const override;
};

class JsonSuffixClient : public QtRestClient::RestClient
{
public:
	JsonSuffixClient(QObject *parent);
	QtRestClient::RequestBuilder builder() const override;

private:
	class Extender : public QtRestClient::RequestBuilder::IExtender
	{
	public:
		void extendUrl(QUrl &url) const override;
	};
};

Q_DECLARE_LOGGING_CATEGORY(logRmc)

}

Q_DECLARE_METATYPE(std::optional<QtDataSync::firebase::realtimedb::Data>)

#endif // QTDATASYNC_REMOTECONNECTOR_H
