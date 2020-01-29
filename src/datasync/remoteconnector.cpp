#include "remoteconnector_p.h"
#include "engine_p.h"
#include <QtCore/QAtomicInteger>
#include <QtCore/QRegularExpression>
#include <QtJsonSerializer/JsonSerializer>
using namespace QtDataSync;
using namespace QtDataSync::__private;
using namespace QtDataSync::firebase;
using namespace QtDataSync::firebase::realtimedb;
using namespace std::chrono;
using namespace std::chrono_literals;

Q_LOGGING_CATEGORY(QtDataSync::logRmc, "qt.datasync.RemoteConnector")

#define CANCEL_IF(token) if (!_cancelCache.remove(cancelToken)) return

const QString RemoteConnector::DeviceIdKey = QStringLiteral("rmc/deviceId");

RemoteConnector::RemoteConnector(const SetupPrivate::FirebaseConfig &config, QNetworkAccessManager *nam, QSettings *settings, QObject *parent) :
	QObject{parent},
	_settings{settings},
	_client{new JsonSuffixClient{this}}
{
#ifdef Q_ATOMIC_INT8_IS_SUPPORTED
	static QAtomicInteger<bool> rmcReg = false;
#else
	static QAtomicInteger<quint16> rmcReg = false;
#endif
	if (rmcReg.testAndSetOrdered(false, true)) {
		// register with name, is as they are typdefs of standard Qt types, not just internal ones
		qRegisterMetaType<Content>("QtDataSync::firebase::realtimedb::Content");
		qRegisterMetaType<Version>("QtDataSync::firebase::realtimedb::Version");

		QtJsonSerializer::SerializerBase::registerVariantConverters<QDateTime, ServerTimestamp>();
		QtJsonSerializer::SerializerBase::registerOptionalConverters<QJsonObject>();
		QtJsonSerializer::SerializerBase::registerVariantConverters<std::optional<QJsonObject>, bool>();
		QtJsonSerializer::SerializerBase::registerOptionalConverters<QVersionNumber>();
		QtJsonSerializer::SerializerBase::registerOptionalConverters<Data>();
	}

	_limit = config.readLimit;
	_syncTableVersions = config.syncTableVersions;

	connect(nam, &QNetworkAccessManager::networkAccessibleChanged,
			this, [this](){
				Q_EMIT onlineChanged(isOnline());
			});

	_client->setManager(nam);
	const auto serializer = _client->serializer();
	serializer->setAllowDefaultNull(true);
	serializer->addJsonTypeConverter<ServerTimestampConverter>();
	serializer->addJsonTypeConverter<AccurateTimestampConverter>();
	serializer->addJsonTypeConverter<QueryMapConverter>();
	_client->setModernAttributes();
	_client->addRequestAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
	_client->setBaseUrl(QUrl{QStringLiteral("https://%1.firebaseio.com/datasync")
								 .arg(config.projectId)});
	_client->addGlobalParameter(QStringLiteral("timeout"), timeString(config.readTimeOut));
	_client->addGlobalParameter(QStringLiteral("writeSizeLimit"), QStringLiteral("unlimited"));
}

bool RemoteConnector::isActive() const
{
	return _api;
}

void RemoteConnector::setUser(QString userId)
{
	if (_api)
		logOut(false);
	_api = new ApiClient{_client->createClass(userId, this), this};
}

void RemoteConnector::setIdToken(const QString &idToken)
{
	_client->removeGlobalParameter(QStringLiteral("auth"));
	_client->addGlobalParameter(QStringLiteral("auth"), idToken);
}

RemoteConnector::CancallationToken RemoteConnector::startLiveSync(const QString &type, const QDateTime &since)
{
	if (!_api) {
		qCWarning(logRmc) << "Unable to start live sync - user is not logged in yet";
		Q_EMIT networkError(type, false);
		return InvalidToken;
	}

	const auto eventStream = _api->restClass()->builder()
								 .addPath(type)
								 .addParameter(QStringLiteral("orderBy"), QStringLiteral("\"uploaded\""))
								 .addParameter(QStringLiteral("startAt"), QString::number(since.toUTC().toMSecsSinceEpoch()))
								 .setAccept("text/event-stream")
								 .send();
	const auto cancelToken = acquireToken(eventStream);
	connect(eventStream, &QNetworkReply::readyRead,
			this, std::bind(&RemoteConnector::streamEvent, this, type, cancelToken));
	connect(eventStream, &QNetworkReply::finished,
			this, std::bind(&RemoteConnector::streamClosed, this, type, cancelToken),
			Qt::QueuedConnection);  // make sure finished is processed after readyRead
	return cancelToken;

}

RemoteConnector::CancallationToken RemoteConnector::getChanges(const QString &type, const QDateTime &since, const CancallationToken continueToken)
{
	if (!_api) {
		qCWarning(logRmc) << "Unable to get changes - user is not logged in yet";
		Q_EMIT networkError(type, false);
		return InvalidToken;
	}
	const auto reply = _api->listChangedData(type, since.toUTC().toMSecsSinceEpoch(), _limit);
	const auto cancelToken = acquireToken(reply, continueToken);
	reply->onSucceeded(this, [this, type, since, cancelToken](int, const QueryMap &data) {
		CANCEL_IF(cancelToken);
		// get all changed data and pass to storage
		QList<CloudData> dlList;
		dlList.reserve(data.size());
		for (auto it = data.begin(), end = data.end(); it != end; ++it) {
			if (it->second.device() != deviceId())
				dlList.append(dlData({type, it->first}, it->second));
		}
		qCDebug(logRmc) << "listChangedData returned" << data.size() << "entries"
						<< "of which" << dlList.size() << "are new for this device";
		if (!dlList.isEmpty())
			Q_EMIT downloadedData(type, dlList);
		// if as much data as possible by limit, fetch more with new last sync
		if (data.size() == _limit)
			getChanges(type, std::get<QDateTime>(data.last().second.uploaded()), cancelToken);
		else
			Q_EMIT syncDone(type);
	})->onAllErrors(this, [this, type, cancelToken](const QString &error, int code, QtRestClient::RestReply::Error errorType) {
		CANCEL_IF(cancelToken);
		evalNetError(error, code, errorType, type);
	}, &RemoteConnector::translateError);
	return cancelToken;
}

RemoteConnector::CancallationToken RemoteConnector::uploadChange(const CloudData &data)
{
	if (!_api) {
		qCWarning(logRmc) << "Unable to upload change - user is not logged in yet";
		Q_EMIT networkError(data.key().tableName, false);
		return InvalidToken;
	}
	const auto reply = _api->getData(data.key().tableName, data.key().key);
	const auto cancelToken = acquireToken(reply);
	reply->onSucceeded(this, [this, data, reply, cancelToken](int, const std::optional<Data> &replyData) {
		CANCEL_IF(cancelToken);
		if (!replyData) {
			// no data on the server -> upload with null tag
			doUpload(data, ApiBase::NullETag, cancelToken);
		} else if (replyData->modified() == data.modified() &&
				   replyData->device() == deviceId()) {
			// data is same as local (same devId and tstamp) -> emit done
			Q_EMIT uploadedData(data.key(), data.modified());
		} else if (replyData->modified() >= data.modified()) {
			// data was modified on the server after modified locally -> ignore upload and treat as download, trigger sync
			qCDebug(logRmc) << "Server data is newer than local data when trying to upload - triggering sync";
			Q_EMIT downloadedData(data.key().tableName, {dlData(data.key(), *replyData, true)});
			Q_EMIT triggerSync(data.key().tableName);
		} else {
			// data on server is older -> upload
			doUpload(data, reply->networkReply()->rawHeader("ETag"), cancelToken);
		}
	})->onAllErrors(this, [this, type = data.key().tableName, cancelToken](const QString &error, int code, QtRestClient::RestReply::Error errorType) {
		CANCEL_IF(cancelToken);
		evalNetError(error, code, errorType, type);
	}, &RemoteConnector::translateError);
	return cancelToken;
}

RemoteConnector::CancallationToken RemoteConnector::removeTable(const QString &type)
{
	if (!_api) {
		qCWarning(logRmc) << "Unable to remove table - user is not logged in yet";
		Q_EMIT networkError(type, false);
		return InvalidToken;
	}
	const auto reply = _api->removeTable(type);
	const auto cancelToken = acquireToken(reply);
	reply->onSucceeded(this, [this, type, cancelToken](int) {
		CANCEL_IF(cancelToken);
		Q_EMIT removedTable(type);
	})->onAllErrors(this, [this, type, cancelToken](const QString &error, int code, QtRestClient::RestReply::Error errorType) {
		CANCEL_IF(cancelToken);
		evalNetError(error, code, errorType, type);
	}, &RemoteConnector::translateError);
	return cancelToken;
}

void RemoteConnector::logOut(bool clearDevId)
{
	if (_api) {
		_api->deleteLater();
		_api = nullptr;
	}
	if (clearDevId) {
		_devId = {};
		_settings->remove(DeviceIdKey);
	}
}

RemoteConnector::CancallationToken RemoteConnector::removeUser()
{
	if (!_api) {
		qCWarning(logRmc) << "Unable to remove all user tables - user is not logged in yet";
		Q_EMIT networkError({}, false);
		return InvalidToken;
	}
	const auto reply = _api->removeUser();
	const auto cancelToken = acquireToken(reply);
	reply->onSucceeded(this, [this, cancelToken](int) {
		CANCEL_IF(cancelToken);
		logOut();
		Q_EMIT removedUser();
	})->onAllErrors(this, [this, cancelToken](const QString &error, int code, QtRestClient::RestReply::Error errorType) {
		CANCEL_IF(cancelToken);
		evalNetError(error, code, errorType, QString{});
	}, &RemoteConnector::translateError);
	return cancelToken;
}

void RemoteConnector::cancel(RemoteConnector::CancallationToken token)
{
	auto reply = _cancelCache.take(token);
	if (reply && !reply->isFinished())
		reply->abort();
}

void RemoteConnector::cancel(const QList<CancallationToken> &tokenList)
{
	for (auto token : tokenList)
		cancel(token);
}

bool RemoteConnector::isOnline() const
{
	switch (_client->manager()->networkAccessible()) {
	case QNetworkAccessManager::NotAccessible:
		return false;
	case QNetworkAccessManager::Accessible:
	case QNetworkAccessManager::UnknownAccessibility:
		return true;
	default:
		Q_UNREACHABLE();
	}
}

void RemoteConnector::evalNetError(const QString &errorString, int errorCode, QtRestClient::RestReply::Error errorType, std::optional<QString> type)
{
	qCWarning(logRmc) << "Realtime-database request failed with reason" << errorType
					  << "and code" << errorCode
					  << "- error message:" << qUtf8Printable(errorString);

	if (type) {
		if (errorType == QtRestClient::RestReply::Error::Network) {
			switch (static_cast<QNetworkReply::NetworkError>(errorCode)) {
			case QNetworkReply::NoError:
			case QNetworkReply::ConnectionRefusedError:
			case QNetworkReply::RemoteHostClosedError:
			case QNetworkReply::HostNotFoundError:
			case QNetworkReply::TimeoutError:
			//case QNetworkReply::SslHandshakeFailedError:
			case QNetworkReply::TemporaryNetworkFailureError:
			case QNetworkReply::NetworkSessionFailedError:
			case QNetworkReply::BackgroundRequestNotAllowedError:
			case QNetworkReply::UnknownNetworkError:
			case QNetworkReply::ProxyConnectionRefusedError:
			case QNetworkReply::ProxyConnectionClosedError:
			case QNetworkReply::ProxyNotFoundError:
			case QNetworkReply::ProxyTimeoutError:
			case QNetworkReply::UnknownProxyError:
			case QNetworkReply::ProtocolFailure:
			case QNetworkReply::InternalServerError:
			case QNetworkReply::ServiceUnavailableError:
			case QNetworkReply::UnknownServerError:
				Q_EMIT networkError(*type, true);
				break;
			default:
				Q_EMIT networkError(*type, false);
				break;
			}
		} else
			Q_EMIT networkError(*type, false);
	}
}

void RemoteConnector::streamEvent(const QString &type, const CancallationToken cancelToken)
{
	// get ev stream
	auto eventStream = qobject_cast<QNetworkReply*>(sender());
	Q_ASSERT_X(eventStream, Q_FUNC_INFO, "Unexpected sender - not a QNetworkReply");
	// verify not canceled yet
	if (!_cancelCache.contains(cancelToken))
		return;
	// process data
	auto &eventData = _eventCache[type];
	while (eventStream->canReadLine()) {
		const auto line = eventStream->readLine().trimmed();
		if (line.startsWith("event: "))
			eventData.event = line.mid(7);
		else if (line.startsWith("data: "))
			eventData.data.append(parseEventData(line.mid(6)));
		else if (line.isEmpty()) {
			processStreamEvent(type, eventData, cancelToken);
		} else
			qCWarning(logRmc).nospace() << "Unexpected event-stream data for " << type << ": " << line;
	}
}

void RemoteConnector::streamClosed(const QString &type, const CancallationToken cancelToken)
{
	// get ev stream (with scope to delete after this method
	QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> eventStream{qobject_cast<QNetworkReply*>(sender())};
	Q_ASSERT_X(eventStream, Q_FUNC_INFO, "Unexpected sender - not a QNetworkReply");
	// verify not canceled yet
	CANCEL_IF(cancelToken);

	// clear event cache
	_eventCache.remove(type);
	// create an error event
	if (eventStream->error() == QNetworkReply::NoError) {
		evalNetError(eventStream->errorString(),
					 eventStream->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(),
					 QtRestClient::RestReply::Error::Failure,
					 type);
	} else {
		evalNetError(eventStream->errorString(),
					 eventStream->error(),
					 QtRestClient::RestReply::Error::Network,
					 type);
	}
}

QString RemoteConnector::translateError(const Error &error, int)
{
	return error.error();
}

QUuid RemoteConnector::deviceId()
{
	if (_devId.isNull()) {
		if (!_settings->contains(DeviceIdKey)) {
			_settings->setValue(DeviceIdKey, QUuid::createUuid());
			_settings->sync();
		}
		_devId = _settings->value(DeviceIdKey).toUuid();
	}
	return _devId;
}

CloudData RemoteConnector::dlData(DatasetId key, const Data &data, bool skipUploaded) const
{
	return {
		std::move(key),
		std::get<std::optional<QJsonObject>>(data.data()),
		data.modified(),
		_syncTableVersions ? data.version() : std::nullopt,
		skipUploaded ? QDateTime{} : std::get<QDateTime>(data.uploaded())
	};
}

RemoteConnector::CancallationToken RemoteConnector::acquireToken(QNetworkReply *reply, const CancallationToken overwriteToken)
{
	const auto token = overwriteToken == InvalidToken ?
		(_cancelCtr == InvalidToken ? ++_cancelCtr : _cancelCtr++) :
		overwriteToken;
	_cancelCache.insert(token, reply);
	return token;
}

QString RemoteConnector::timeString(const milliseconds &time)
{
	if (const auto minValue = duration_cast<minutes>(time); minValue == time)
		return QStringLiteral("%1min").arg(minValue.count());
	else if (const auto secValue = duration_cast<seconds>(time); secValue == time)
		return QStringLiteral("%1s").arg(secValue.count());
	else
		return QStringLiteral("%1ms").arg(time.count());
}

void RemoteConnector::doUpload(const CloudData &data, QByteArray eTag, CancallationToken cancelToken)
{
	if (!_api) {
		qCWarning(logRmc) << "Unable to upload data - user is not logged in yet";
		Q_EMIT networkError(data.key().tableName, false);
		return;
	}
	// data on server is older -> upload
	ApiBase::ETagSetter eTagSetter{_api, std::move(eTag)};
	const auto reply = _api->uploadData({
											data.data(),
											data.modified(),
											_syncTableVersions ? data.version() : std::nullopt,
											ServerTimestamp{},
											deviceId()
										},
										data.key().tableName,
										data.key().key);
	cancelToken = acquireToken(reply, cancelToken);
	reply->onSucceeded(this, [this, key = data.key(), mod = data.modified(), cancelToken](int) {
		CANCEL_IF(cancelToken);
		Q_EMIT uploadedData(key, mod);
	})->onAllErrors(this, [this, type = data.key().tableName, cancelToken](const QString &error, int code, QtRestClient::RestReply::Error errorType){
		CANCEL_IF(cancelToken);
		if (errorType == QtRestClient::RestReply::Error::Failure && code == CodeETagMismatch) {
			// data was changed on the server since checked -> trigger sync
			qCDebug(logRmc) << "Server data was changed since the ETag was requested - triggering sync";
			Q_EMIT triggerSync(type);
		} else
			evalNetError(error, code, errorType, type);
	}, &RemoteConnector::translateError);
}

QJsonValue RemoteConnector::parseEventData(const QByteArray &data)
{
	QJsonParseError error;
	auto jDoc = QJsonDocument::fromJson(data, &error);
	switch (error.error) {
	case QJsonParseError::NoError:
		if (jDoc.isObject())
			return jDoc.object();
		else if (jDoc.isArray())
			return jDoc.array();
		else
			return QJsonValue::Null;
	case QJsonParseError::IllegalValue:
		error = QJsonParseError {};  // clear error
		jDoc = QJsonDocument::fromJson("[" + data + "]", &error);  // read wrapped as array
		if (error.error == QJsonParseError::NoError)
			return jDoc.array().first();
		else
			return QJsonValue::Null;
	default:
		return QJsonValue::Null;
	}
}

void RemoteConnector::processStreamEvent(const QString &type, EventData &data, CancallationToken cancelToken)
{
	if (data.event == "put") {
		qCDebug(logRmc) << "Received event-stream put event";
		const auto serializer = static_cast<QtJsonSerializer::JsonSerializer*>(_client->serializer());
		for (const auto &data : qAsConst(data.data)) {
			const auto dataObj = data.toObject();
			const auto evPath = dataObj[QStringLiteral("path")].toString();
			const auto evData = dataObj[QStringLiteral("data")].toObject();
			try {
				if (evPath == QStringLiteral("/")) {
					auto initData = serializer->deserialize<QueryMap>(evData);
					QList<CloudData> dlList;
					dlList.reserve(initData.size());
					for (auto it = initData.begin(), end = initData.end(); it != end; ++it) {
						if (it->second.device() != deviceId())
							dlList.append(dlData({type, it->first}, it->second));
					}
					qCDebug(logRmc) << "liveSync initialized with" << initData.size() << "entries"
									<< "of which" << dlList.size() << "are new for this device";
					if (!dlList.isEmpty())
						Q_EMIT downloadedData(type, dlList);
					Q_EMIT syncDone(type);
				} else {
					static const QRegularExpression pathRegexp {QStringLiteral(R"__(^\/([^\/]+)$)__")};
					if (const auto match = pathRegexp.match(evPath); match.hasMatch()) {
						const auto data = serializer->deserialize<Data>(evData);
						if (data.device() != deviceId())
							Q_EMIT downloadedData(type, {dlData({type, match.captured(1)}, data)});
						else
							qCDebug(logRmc) << "Skipping processing of live data from this device";
					} else
						qCDebug(logRmc) << "Ignoring put event of" << type << "for unsupported data path" << evPath;
				}
			} catch (QtJsonSerializer::DeserializationException &e) {
				qCWarning(logRmc) << "Livesync data with type" << type
								  << "and path" << evPath
								  << "triggered serialization error:" << e.what();
				cancel(cancelToken);
				Q_EMIT networkError(type, true);
			}
		}
	} else if (data.event == "keep-alive")
		qCDebug(logRmc) << "Received event-stream keep-alive event";
	else if (data.event == "cancel") {
		for (const auto &data : qAsConst(data.data))
			qCWarning(logRmc).noquote() << "Event-stream canceled with reason:" << data.toString();
		cancel(cancelToken);
		Q_EMIT networkError(type, true);
	} else if (data.event == "auth_revoked") {
		qCDebug(logRmc) << "Event-stream closed because the authentication expired - reconnecting…";
		cancel(cancelToken);
		Q_EMIT liveSyncExpired(type);
	} else
		qCWarning(logRmc) << "Unsupported event-stream event:" << data.event;

	data.reset();
}



void RemoteConnector::EventData::reset()
{
	data.clear();
	event.clear();
}



AccurateTimestampConverter::AccurateTimestampConverter()
{
	setPriority(High);
}

bool AccurateTimestampConverter::canConvert(int metaTypeId) const
{
	return metaTypeId == QMetaType::QDateTime;
}

QList<QCborValue::Type> AccurateTimestampConverter::allowedCborTypes(int metaTypeId, QCborTag tag) const
{
	Q_UNUSED(metaTypeId)
	Q_UNUSED(tag)
	return {QCborValue::Integer, QCborValue::Double};
}

QCborValue AccurateTimestampConverter::serialize(int propertyType, const QVariant &value) const
{
	Q_UNUSED(propertyType)
	return value.toDateTime().toUTC().toMSecsSinceEpoch();
}

QVariant AccurateTimestampConverter::deserializeCbor(int propertyType, const QCborValue &value, QObject *parent) const
{
	Q_UNUSED(propertyType)
	Q_UNUSED(parent)
	return QDateTime::fromMSecsSinceEpoch(value.toInteger(), Qt::UTC);
}



JsonSuffixClient::JsonSuffixClient(QObject *parent) :
	RestClient{DataMode::Json, parent}
{}

QtRestClient::RequestBuilder JsonSuffixClient::builder() const
{
	return RestClient::builder()
		.trailingSlash(false)
		.setExtender(new Extender{});
}

void JsonSuffixClient::Extender::extendUrl(QUrl &url) const
{
	url.setPath(url.path() + QStringLiteral(".json"));
}
