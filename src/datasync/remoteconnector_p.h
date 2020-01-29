#ifndef QTDATASYNC_REMOTECONNECTOR_H
#define QTDATASYNC_REMOTECONNECTOR_H

#include "qtdatasync_global.h"
#include "engine.h"
#include "datasetid.h"
#include "cloudtransformer.h"
#include "setup_impl.h"

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QLoggingCategory>

#include "realtimedb_api.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT RemoteConnector : public QObject
{
	Q_OBJECT

	Q_PROPERTY(bool online READ isOnline NOTIFY onlineChanged)

public:
	using CancallationToken = quint64;
	static constexpr CancallationToken InvalidToken = std::numeric_limits<CancallationToken>::max();

	static constexpr int CodeNotFound = 404;
	static constexpr int CodeETagMismatch = 412;

	static const QString DeviceIdKey;

	explicit RemoteConnector(const __private::SetupPrivate::FirebaseConfig &config,
							 QNetworkAccessManager *nam,
							 QSettings *settings,
							 QObject *parent);

	bool isActive() const;
	void setUser(QString userId);
	void setIdToken(const QString &idToken);

	CancallationToken startLiveSync(const QString &type, const QDateTime &since);
	CancallationToken getChanges(const QString &type, const QDateTime &since, const CancallationToken continueToken = InvalidToken);
	CancallationToken uploadChange(const CloudData &data);
	CancallationToken removeTable(const QString &type);
	void logOut(bool clearDevId = true);
	CancallationToken removeUser();

	void cancel(CancallationToken token);
	void cancel(const QList<CancallationToken> &tokenList);

	bool isOnline() const;

Q_SIGNALS:
	void downloadedData(const QString &type, const QList<QtDataSync::CloudData> &data, QPrivateSignal = {});
	void syncDone(const QString &type, QPrivateSignal = {});
	void uploadedData(const QtDataSync::DatasetId &key, const QDateTime &modified, QPrivateSignal = {});
	void triggerSync(const QString &type, QPrivateSignal = {});
	void removedTable(const QString &type, QPrivateSignal = {});
	void removedUser(QPrivateSignal = {});

	void networkError(const QString &type, bool temporary, QPrivateSignal = {});
	void liveSyncExpired(const QString &type, QPrivateSignal = {});

	void onlineChanged(bool online, QPrivateSignal = {});

private Q_SLOTS:
	void evalNetError(const QString &errorString,
					  int errorCode,
					  QtRestClient::RestReply::Error errorType,
					  std::optional<QString> type = std::nullopt);

	void streamEvent(const QString &type, const CancallationToken cancelToken);
	void streamClosed(const QString &type, const CancallationToken cancelToken);

private:
	struct EventData {
		QByteArray event;
		QList<QJsonValue> data;

		void reset();
	};

	QPointer<QSettings> _settings;
	QtRestClient::RestClient *_client;
	firebase::realtimedb::ApiClient *_api = nullptr;
	int _limit = 100;
	bool _syncTableVersions = false;

	CancallationToken _cancelCtr = 0;
	QHash<CancallationToken, QPointer<QNetworkReply>> _cancelCache;

	QUuid _devId;
	QHash<QString, EventData> _eventCache;

	static QString translateError(const QtDataSync::firebase::realtimedb::Error &error, int code);

	QUuid deviceId();
	CloudData dlData(DatasetId key, const firebase::realtimedb::Data &data, bool skipUploaded = false) const;
	CancallationToken acquireToken(QNetworkReply *reply, const CancallationToken overwriteToken = InvalidToken);
	inline CancallationToken acquireToken(QtRestClient::RestReply *reply, const CancallationToken overwriteToken = InvalidToken) {
		return acquireToken(reply->networkReply(), overwriteToken);
	}
	QString timeString(const std::chrono::milliseconds &time);
	void doUpload(const CloudData &data, QByteArray eTag, CancallationToken cancelToken);

	QJsonValue parseEventData(const QByteArray &data);
	void processStreamEvent(const QString &type, EventData &data, CancallationToken cancelToken);
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
