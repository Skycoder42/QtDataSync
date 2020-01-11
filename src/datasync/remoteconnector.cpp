#include "remoteconnector_p.h"
#include "engine_p.h"
#include <QtCore/QAtomicInteger>
#include <QtCore/QRegularExpression>
using namespace QtDataSync;
using namespace QtDataSync::firebase;
using namespace QtDataSync::firebase::realtimedb;
using namespace std::chrono;
using namespace std::chrono_literals;

Q_LOGGING_CATEGORY(QtDataSync::logRmc, "qt.datasync.RemoteConnector")

const QByteArray RemoteConnector::NullETag {"null_etag"};

#define CANCEL_IF(token) if (!_cancelCache.remove(cancelToken)) return

RemoteConnector::RemoteConnector(Engine *engine) :
	QObject{engine}
{
#ifdef Q_ATOMIC_INT8_IS_SUPPORTED
	static QAtomicInteger<bool> rmcReg = false;
#else
	static QAtomicInteger<quint16> rmcReg = false;
#endif
	if (rmcReg.testAndSetOrdered(false, true)) {
		qRegisterMetaType<ServerTimestamp>("ServerTimestamp");
		qRegisterMetaType<Timestamp>("Timestamp");
		qRegisterMetaType<Content>("Content");

		QtJsonSerializer::SerializerBase::registerVariantConverters<QDateTime, ServerTimestamp>();
		QtJsonSerializer::SerializerBase::registerOptionalConverters<QJsonObject>();
		QtJsonSerializer::SerializerBase::registerVariantConverters<std::optional<QJsonObject>, bool>();
		QtJsonSerializer::SerializerBase::registerOptionalConverters<Data>();
	}

	const auto setup = EnginePrivate::setupFor(engine);
	_limit = setup->firebase.readLimit;

	_client = new JsonSuffixClient{this};
	const auto serializer = _client->serializer();
	serializer->setAllowDefaultNull(true);
	serializer->addJsonTypeConverter<ServerTimestampConverter>();
	serializer->addJsonTypeConverter<AccurateTimestampConverter>();
	serializer->addJsonTypeConverter<QueryMapConverter>();
	_client->manager()->setStrictTransportSecurityEnabled(true);
	_client->setModernAttributes();
	_client->addRequestAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
	_client->setBaseUrl(QUrl{QStringLiteral("https://%1.firebaseio.com/datasync")
								 .arg(setup->firebase.projectId)});
	_client->addGlobalParameter(QStringLiteral("timeout"), timeString(setup->firebase.readTimeOut));
	_client->addGlobalParameter(QStringLiteral("writeSizeLimit"), QStringLiteral("unlimited"));
}

bool RemoteConnector::isActive() const
{
	return _api;
}

void RemoteConnector::setUser(QString userId)
{
	if (_api)
		_api->deleteLater();
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
		Q_EMIT networkError(tr("User is not logged in yet"), type);
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
		Q_EMIT networkError(tr("User is not logged in yet"), type);
		return InvalidToken;
	}
	const auto reply = _api->listChangedData(type, since.toUTC().toMSecsSinceEpoch(), _limit);
	const auto cancelToken = acquireToken(reply, continueToken);
	reply->onSucceeded(this, [this, type, since, cancelToken](int, const QueryMap &data) {
		CANCEL_IF(cancelToken);
		// get all changed data and pass to storage
		qCDebug(logRmc) << "listChangedData returned" << data.size() << "entries";
		if (!data.isEmpty()) {
			QList<CloudData> dlList;
			dlList.reserve(data.size());
			for (auto it = data.begin(), end = data.end(); it != end; ++it)
				dlList.append(dlData({type, it->first}, it->second));
			Q_EMIT downloadedData(type, dlList);
		}
		// if as much data as possible by limit, fetch more with new last sync
		if (data.size() == _limit)
			getChanges(type, std::get<QDateTime>(data.last().second.uploaded()), cancelToken);
		else
			Q_EMIT syncDone(type);
	})->onAllErrors(this, [this, type, cancelToken](const QString &error, int code, QtRestClient::RestReply::Error errorType) {
		CANCEL_IF(cancelToken);
		apiError(error, code, errorType);
		Q_EMIT networkError(tr("Failed to download latests changed data"), type);
	}, &RemoteConnector::translateError);
	return cancelToken;
}

RemoteConnector::CancallationToken RemoteConnector::uploadChange(const CloudData &data)
{
	if (!_api) {
		Q_EMIT networkError(tr("User is not logged in yet"), data.key().typeName);
		return InvalidToken;
	}
	const auto reply = _api->getData(data.key().typeName, data.key().id);
	const auto cancelToken = acquireToken(reply);
	reply->onSucceeded(this, [this, data, reply, cancelToken](int, const std::optional<Data> &replyData) {
		CANCEL_IF(cancelToken);
		if (!replyData) {
			// no data on the server -> upload with null tag
			doUpload(data, NullETag, cancelToken);
		} else if (replyData->modified() >= data.modified()) {
			// data was modified on the server after modified locally -> ignore upload
			qCDebug(logRmc) << "Server data is newer than local data when trying to upload - triggering sync";
			Q_EMIT downloadedData(data.key().typeName, {dlData(data.key(), *replyData, true)});
			Q_EMIT triggerSync(data.key().typeName);
		} else {
			// data on server is older -> upload
			doUpload(data, reply->networkReply()->rawHeader("ETag"), cancelToken);
		}
	})->onAllErrors(this, [this, type = data.key().typeName, cancelToken](const QString &error, int code, QtRestClient::RestReply::Error errorType) {
		CANCEL_IF(cancelToken);
		apiError(error, code, errorType);
		Q_EMIT networkError(tr("Failed to verify data version before uploading"), type);
	}, &RemoteConnector::translateError);
	return cancelToken;
}

RemoteConnector::CancallationToken RemoteConnector::removeTable(const QString &type)
{
	if (!_api) {
		Q_EMIT networkError(tr("User is not logged in yet"), type);
		return InvalidToken;
	}
	const auto reply = _api->removeTable(type);
	const auto cancelToken = acquireToken(reply);
	reply->onSucceeded(this, [this, type, cancelToken](int) {
		CANCEL_IF(cancelToken);
		Q_EMIT removedTable(type);
	})->onAllErrors(this, [this, type, cancelToken](const QString &error, int code, QtRestClient::RestReply::Error errorType) {
		CANCEL_IF(cancelToken);
		apiError(error, code, errorType);
		Q_EMIT networkError(tr("Failed to remove table from server"), type);
	}, &RemoteConnector::translateError);
	return cancelToken;
}

RemoteConnector::CancallationToken RemoteConnector::removeUser()
{
	if (!_api) {
		Q_EMIT networkError(tr("User is not logged in yet"), {});
		return InvalidToken;
	}
	const auto reply = _api->removeUser();
	const auto cancelToken = acquireToken(reply);
	reply->onSucceeded(this, [this, cancelToken](int) {
		CANCEL_IF(cancelToken);
		_api->deleteLater();
		_api = nullptr;
		Q_EMIT removedUser();
	})->onAllErrors(this, [this, cancelToken](const QString &error, int code, QtRestClient::RestReply::Error errorType) {
		CANCEL_IF(cancelToken);
		apiError(error, code, errorType);
		Q_EMIT networkError(tr("Failed to delete user from server"), {});
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

void RemoteConnector::apiError(const QString &errorString, int errorCode, QtRestClient::RestReply::Error errorType)
{
	qCWarning(logRmc) << "Realtime-database request failed with reason" << errorType
					  << "and code" << errorCode
					  << "- error message:" << qUtf8Printable(errorString);
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
	// clear event cache
	_eventCache.remove(type);
	// verify not canceled yet
	CANCEL_IF(cancelToken);

	const auto code = eventStream->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	qCWarning(logRmc) << "Event stream broken with error code" << code
					  << "and message:" << qUtf8Printable(eventStream->errorString());

	switch (eventStream->error()) {
	// acceptable error codes to retry to connect
	case QNetworkReply::NoError:
	case QNetworkReply::RemoteHostClosedError:
	case QNetworkReply::TimeoutError:
	case QNetworkReply::TemporaryNetworkFailureError:
	case QNetworkReply::NetworkSessionFailedError:
	case QNetworkReply::ProxyConnectionClosedError:
	case QNetworkReply::ProxyTimeoutError:
	case QNetworkReply::InternalServerError:
	case QNetworkReply::ServiceUnavailableError:
	case QNetworkReply::UnknownNetworkError:
	case QNetworkReply::UnknownProxyError:
	case QNetworkReply::ProtocolFailure:
	case QNetworkReply::UnknownServerError:
	// stopped by user -> abort was called -> soft reconnect
	case QNetworkReply::OperationCanceledError:
		Q_EMIT liveSyncError(tr("Live-synchronization connection broken with recoverable error!"), type, true);
		break;
	// inacceptable error codes that indicate a hard failure
	default:
		Q_EMIT liveSyncError(tr("Live-synchronization connection broken with unrecoverable error!"), type, false);
		break;
	}
}

CloudData RemoteConnector::dlData(ObjectKey key, const Data &data, bool skipUploaded)
{
	return {
		std::move(key),
		std::get<std::optional<QJsonObject>>(data.data()),
		data.modified(),
		skipUploaded ? QDateTime{} : std::get<QDateTime>(data.uploaded())
	};
}

QString RemoteConnector::translateError(const Error &error, int)
{
	return error.error();
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
		Q_EMIT networkError(tr("User is not logged in yet"), data.key().typeName);
		return;
	}
	// data on server is older -> upload
	FirebaseApiBase::ETagSetter eTagSetter{_api, std::move(eTag)};
	const auto reply = _api->uploadData({data.data(), data.modified(), ServerTimestamp{}},
										data.key().typeName,
										data.key().id);
	cancelToken = acquireToken(reply, cancelToken);
	reply->onSucceeded(this, [this, key = data.key(), mod = data.modified(), cancelToken](int) {
		CANCEL_IF(cancelToken);
		Q_EMIT uploadedData(key, mod);
	})->onAllErrors(this, [this, type = data.key().typeName, cancelToken](const QString &error, int code, QtRestClient::RestReply::Error errorType){
		CANCEL_IF(cancelToken);
		if (errorType == QtRestClient::RestReply::Error::Failure && code == CodeETagMismatch) {
			// data was changed on the server since checked -> trigger sync
			qCDebug(logRmc) << "Server data was changed since the ETag was requested - triggering sync";
			Q_EMIT triggerSync(type);
		} else {
			apiError(error, code, errorType);
			Q_EMIT networkError(tr("Failed to upload data"), type);
		}
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
					if (!initData.isEmpty()) {
						QList<CloudData> dlList;
						dlList.reserve(initData.size());
						for (auto it = initData.begin(), end = initData.end(); it != end; ++it)
							dlList.append(dlData({type, it->first}, it->second));
						Q_EMIT downloadedData(type, dlList);
					}
					Q_EMIT syncDone(type);
				} else {
					static const QRegularExpression pathRegexp {QStringLiteral(R"__(^\/([^\/]+)$)__")};
					if (const auto match = pathRegexp.match(evPath); match.hasMatch()) {
						Q_EMIT downloadedData(type, {dlData(
														{type, match.captured(1)},
														serializer->deserialize<Data>(evData))
													});
					} else
						qCDebug(logRmc) << "Ignoring put event of" << type << "for unsupported data path" << evPath;
				}
			} catch (QtJsonSerializer::DeserializationException &e) {
				qCWarning(logRmc) << "Livesync data with type" << type
								  << "and path" << evPath
								  << "triggered serialization error:" << e.what();
				cancel(cancelToken);
				Q_EMIT liveSyncError(tr("Live-synchronization received invalid data"), type, true);
			}
		}
	} else if (data.event == "keep-alive")
		qCDebug(logRmc) << "Received event-stream keep-alive event";
	else if (data.event == "cancel") {
		for (const auto &data : qAsConst(data.data))
			qCWarning(logRmc).noquote() << "Event-stream canceled with reason:" << data.toString();
		cancel(cancelToken);
		Q_EMIT liveSyncError(tr("Live-synchronization was stopped by the remote server"), type, true);
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
