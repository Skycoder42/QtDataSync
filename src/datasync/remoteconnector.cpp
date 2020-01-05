#include "remoteconnector_p.h"
#include "engine_p.h"
#include <QtCore/QAtomicInteger>
using namespace QtDataSync;
using namespace QtDataSync::firebase;
using namespace QtDataSync::firebase::realtimedb;
using namespace std::chrono;
using namespace std::chrono_literals;

Q_LOGGING_CATEGORY(QtDataSync::logRmc, "qt.datasync.RemoteConnector")

const QByteArray RemoteConnector::NullETag {"null_etag"};

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
	_userId = std::move(userId);
	_api = new ApiClient{_client->createClass(_userId, this), this};
}

void RemoteConnector::setIdToken(const QString &idToken)
{
	_client->removeGlobalParameter(QStringLiteral("auth"));
	_client->addGlobalParameter(QStringLiteral("auth"), idToken);
}

void RemoteConnector::startLiveSync()
{
	if (_eventStream)
		stopLiveSync();

	_eventStream = _client->builder()
					   .addPath(_userId)
					   .setAccept("text/event-stream")
					   .setVerb(QtRestClient::RestClass::GetVerb)
					   .send();
	connect(_eventStream, &QNetworkReply::readyRead,
			this, &RemoteConnector::streamEvent);
	connect(_eventStream, &QNetworkReply::finished,
			this, &RemoteConnector::streamClosed);

}

void RemoteConnector::stopLiveSync()
{
	if (_eventStream) {
		_eventStream->disconnect(this);
		if (_eventStream->isFinished())
			_eventStream->deleteLater();
		else {
			connect(_eventStream, &QNetworkReply::finished,
					_eventStream, &QNetworkReply::deleteLater);
			_eventStream->abort();
		}
	}
}

void RemoteConnector::getChanges(const QString &type, const QDateTime &since)
{
	if (!_api) {
		Q_EMIT networkError(tr("User is not logged in yet"));
		return;
	}
	_api->listChangedData(type, since.toUTC().toMSecsSinceEpoch(), _limit)->onSucceeded(this, [this, type, since](int, const QueryMap &data) {
		// get all changed data and pass to storage
		qCDebug(logRmc) << "listChangedData returned" << data.size() << "entries";
		if (!data.isEmpty()) {
			QList<CloudData> dlList;
			dlList.reserve(data.size());
			for (auto it = data.begin(), end = data.end(); it != end; ++it)
				dlList.append(dlData({type, it->first}, it->second));
			Q_EMIT downloadedData(dlList, false);
		}
		// if as much data as possible by limit, fetch more with new last sync
		if (data.size() == _limit)
			getChanges(type, std::get<QDateTime>(data.last().second.uploaded()));
		else
			Q_EMIT syncDone(type);
	})->onAllErrors(this, [this](const QString &error, int code, QtRestClient::RestReply::Error errorType){
		apiError(error, code, errorType);
		Q_EMIT networkError(tr("Failed to download latests changed data"));
	}, &RemoteConnector::translateError);
}

void RemoteConnector::uploadChange(const CloudData &data)
{
	if (!_api) {
		Q_EMIT networkError(tr("User is not logged in yet"));
		return;
	}
	auto reply = _api->getData(data.key().typeName, data.key().id);
	reply->onSucceeded(this, [this, data, reply](int, const std::optional<Data> &replyData) {
		if (!replyData) {
			// no data on the server -> upload with null tag
			doUpload(data, NullETag);
		} else if (replyData->modified() >= data.modified()) {
			// data was modified on the server after modified locally -> ignore upload
			qCDebug(logRmc) << "Server data is newer than local data when trying to upload - triggering sync";
			Q_EMIT downloadedData({dlData(data.key(), *replyData, true)}, false);
			Q_EMIT triggerSync(data.key().typeName);
		} else {
			// data on server is older -> upload
			doUpload(data, reply->networkReply()->rawHeader("ETag"));
		}
	})->onAllErrors(this, [this, data](const QString &error, int code, QtRestClient::RestReply::Error errorType) {
		apiError(error, code, errorType);
		Q_EMIT networkError(tr("Failed to verify data version before uploading"));
	}, &RemoteConnector::translateError);
}

void RemoteConnector::removeTable(const QString &type)
{
	if (!_api) {
		Q_EMIT networkError(tr("User is not logged in yet"));
		return;
	}
	_api->removeTable(type)->onSucceeded(this, [this, type](int) {
		Q_EMIT removedTable(type);
	})->onAllErrors(this, [this](const QString &error, int code, QtRestClient::RestReply::Error errorType) {
		apiError(error, code, errorType);
		Q_EMIT networkError(tr("Failed to remove table from server"));
	}, &RemoteConnector::translateError);
}

void RemoteConnector::removeUser()
{
	if (!_api) {
		Q_EMIT networkError(tr("User is not logged in yet"));
		return;
	}
	_api->removeUser()->onSucceeded(this, [this](int) {
		_api->deleteLater();
		_api = nullptr;
		Q_EMIT removedUser();
	})->onAllErrors(this, [this](const QString &error, int code, QtRestClient::RestReply::Error errorType) {
		apiError(error, code, errorType);
		Q_EMIT networkError(tr("Failed to delete user from server"));
	}, &RemoteConnector::translateError);
}

void RemoteConnector::apiError(const QString &errorString, int errorCode, QtRestClient::RestReply::Error errorType)
{
	qCWarning(logRmc) << "Realtime-database request failed with reason" << errorType
					  << "and code" << errorCode
					  << "- error message:" << qUtf8Printable(errorString);
}

void RemoteConnector::streamEvent()
{
	while (_eventStream->canReadLine()) {
		const auto line = _eventStream->readLine().trimmed();
		if (line.startsWith("event: "))
			_lastEvent = line.mid(7);
		else if (line.startsWith("data: "))
			_lastData.append(parseEventData(line.mid(6)));
		else if (line.isEmpty()) {
			processStreamEvent();
		} else
			qCWarning(logRmc) << "Unexpected event-stream data:" << line;
	}
}

void RemoteConnector::streamClosed()
{
	const auto code = _eventStream->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	qCWarning(logRmc) << "Event stream broken with error code" << code
					  << "and message:" << qUtf8Printable(_eventStream->errorString());

	switch (_eventStream->error()) {
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
		// TODO add time delays / fail counter
		_eventStream->deleteLater();
		_eventStream = nullptr;
		Q_EMIT liveSyncError(tr("Live-synchronization connection broken, trying to reconnect…"), true);
		break;
	// inacceptable error codes that indicate a hard failure
	default:
		_eventStream->deleteLater();
		_eventStream = nullptr;
		Q_EMIT liveSyncError(tr("Live-synchronization disabled because of network error!"), false);
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

QString RemoteConnector::timeString(const milliseconds &time)
{
	if (const auto minValue = duration_cast<minutes>(time); minValue == time)
		return QStringLiteral("%1min").arg(minValue.count());
	else if (const auto secValue = duration_cast<seconds>(time); secValue == time)
		return QStringLiteral("%1s").arg(secValue.count());
	else
		return QStringLiteral("%1ms").arg(time.count());
}

void RemoteConnector::doUpload(const CloudData &data, QByteArray eTag)
{
	if (!_api) {
		Q_EMIT networkError(tr("User is not logged in yet"));
		return;
	}
	// data on server is older -> upload
	FirebaseApiBase::ETagSetter eTagSetter{_api, std::move(eTag)};
	_api->uploadData({data.data(), data.modified(), ServerTimestamp{}},
					 data.key().typeName,
					 data.key().id)
		->onSucceeded(this, [this, key = data.key(), mod = data.modified()](int) {
			Q_EMIT uploadedData(key, mod);
		})->onAllErrors(this, [this, type = data.key().typeName](const QString &error, int code, QtRestClient::RestReply::Error errorType){
			if (errorType == QtRestClient::RestReply::Error::Failure && code == CodeETagMismatch) {
				// data was changed on the server since checked -> trigger sync
				qCDebug(logRmc) << "Server data was changed since the ETag was requested - triggering sync";
				Q_EMIT triggerSync(type);
			} else {
				apiError(error, code, errorType);
				Q_EMIT networkError(tr("Failed to upload data"));
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

void RemoteConnector::processStreamEvent()
{
	if (_lastEvent == "put") {
		qCDebug(logRmc) << "Received event-stream put event";
		const auto serializer = static_cast<QtJsonSerializer::JsonSerializer*>(_client->serializer());
		for (const auto &data : qAsConst(_lastData)) {
			const auto streamData = serializer->deserialize<StreamData>(data.toObject());
			const auto pElements = streamData.path.split(QLatin1Char('/'), QString::SkipEmptyParts);
			if (pElements.size() != 2) {
				qCDebug(logRmc) << "Ignoring unsupported stream-event path" << streamData.path;
				continue;
			}
			Q_EMIT downloadedData({dlData({pElements[0], pElements[1]}, streamData.data)}, true);
		}
	} else if (_lastEvent == "keep-alive")
		qCDebug(logRmc) << "Received event-stream keep-alive event";
	else if (_lastEvent == "cancel") {
		for (const auto &data : qAsConst(_lastData))
			qCWarning(logRmc).noquote() << "Event-stream canceled with reason:" << data.toString();
		stopLiveSync();
		Q_EMIT liveSyncError(tr("Live-synchronization was stopped by the remote server"), true);
	} else if (_lastEvent == "auth_revoked") {
		stopLiveSync();
		Q_EMIT liveSyncError(tr("Live-synchronization expired, reconnecting…"), true);
	} else
		qCWarning(logRmc) << "Unsupported event-stream event:" << _lastEvent;

	_lastEvent.clear();
	_lastData.clear();
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
