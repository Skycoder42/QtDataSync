#ifndef QTDATASYNC_REMOTECONNECTOR_H
#define QTDATASYNC_REMOTECONNECTOR_H

#include "qtdatasync_global.h"
#include "engine.h"
#include "objectkey.h"
#include "cloudtransformer.h"

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

	explicit RemoteConnector(Engine *engine);

	bool isActive() const;
	void setUser(QString userId);
	void setIdToken(const QString &idToken);

public Q_SLOTS:
	void startLiveSync();
	void stopLiveSync();

	void getChanges(const QString &type, const QDateTime &since);
	void uploadChange(const CloudData &data);
	void removeTable(const QString &type);
	void removeUser();

Q_SIGNALS:
	void triggerSync(const QString &type, QPrivateSignal = {});

	void downloadedData(const QList<QtDataSync::CloudData> &data, bool liveSyncData, QPrivateSignal = {});
	void syncDone(const QString &type, QPrivateSignal = {});
	void uploadedData(const QtDataSync::ObjectKey &key, const QDateTime &modified, QPrivateSignal = {});
	void removedTable(const QString &type, QPrivateSignal = {});
	void removedUser(QPrivateSignal = {});

	void networkError(const QString &error, QPrivateSignal = {});

private Q_SLOTS:
	void apiError(const QString &errorString, int errorCode, QtRestClient::RestReply::Error errorType);

	void streamEvent();
	void streamClosed();

private:
	QtRestClient::RestClient *_client = nullptr;
	firebase::realtimedb::ApiClient *_api = nullptr;
	int _limit = 100;

	QString _userId;
	QNetworkReply *_eventStream = nullptr;
	QByteArray _lastEvent;
	QList<QJsonValue> _lastData;

	static CloudData dlData(ObjectKey key, const firebase::realtimedb::Data &data);
	static QString translateError(const QtDataSync::firebase::realtimedb::Error &error, int code);

	QString timeString(const std::chrono::milliseconds &time);
	void doUpload(const CloudData &data, QByteArray eTag);

	QJsonValue parseEventData(const QByteArray &data);
	void processStreamEvent();
};

struct StreamData
{
	Q_GADGET

	Q_PROPERTY(QString path MEMBER path)
	Q_PROPERTY(firebase::realtimedb::Data data MEMBER data)

public:
	QString path;
	firebase::realtimedb::Data data;
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
