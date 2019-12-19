#include "remoteconnector_p.h"
#include "engine_p.h"
using namespace QtDataSync;
using namespace QtDataSync::firebase;
using namespace QtDataSync::firebase::realtimedb;
using namespace std::chrono;
using namespace std::chrono_literals;

Q_LOGGING_CATEGORY(QtDataSync::logRmc, "qt.datasync.RemoteConnector")

const QByteArray RemoteConnector::NullETag {"null_etag"};

RemoteConnector::RemoteConnector(const QString &userId, Engine *engine) :
	QObject{engine}
{
	const auto setup = EnginePrivate::setupFor(engine);
	_limit = setup->firebase.readLimit;

	auto client = new JsonSuffixClient{this};
	client->serializer()->setAllowDefaultNull(true);
	client->serializer()->addJsonTypeConverter<ServerTimestampConverter>();
	client->serializer()->addJsonTypeConverter<AccurateTimestampConverter>();
	client->manager()->setStrictTransportSecurityEnabled(true);
	client->setModernAttributes();
	client->addRequestAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
	client->setBaseUrl(QUrl{QStringLiteral("https://%1.firebaseio.com/datasync/%2")
								.arg(setup->firebase.projectId, userId)});
	client->addGlobalParameter(QStringLiteral("timeout"), timeString(setup->firebase.readTimeOut));
	client->addGlobalParameter(QStringLiteral("writeSizeLimit"), QStringLiteral("unlimited"));
	_api = new ApiClient{client->rootClass(), this};
	_api->setErrorTranslator(&RemoteConnector::translateError);
	connect(_api, &ApiClient::apiError,
			this, &RemoteConnector::apiError);
}

void RemoteConnector::setIdToken(const QString &idToken)
{
	auto client = _api->restClient();
	client->removeGlobalParameter(QStringLiteral("auth"));
	client->addGlobalParameter(QStringLiteral("auth"), idToken);
}

void RemoteConnector::startLiveSync()
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

	_liveSyncBlocked = true;
	_eventStream = _api->restClient()->builder()
					   .setAccept("text/event-stream")
					   .send();
	connect(_eventStream, &QNetworkReply::readyRead,
			this, &RemoteConnector::streamEvent);
	connect(_eventStream, &QNetworkReply::finished,
			this, &RemoteConnector::streamClosed);

}

void RemoteConnector::unblockLiveSync()
{
	_liveSyncBlocked = false;
}

void RemoteConnector::stopLiveSync()
{
	if (_eventStream)
		_eventStream->abort();
}

void RemoteConnector::getChanges(const QByteArray &type, const QDateTime &since)
{
	_api->listChangedData(QString::fromUtf8(type), since, _limit)->onSucceeded(this, [this, type, since](int, const QHash<QString, Data> &data) {
		// get all changed data and pass to storage
		auto lasySync = since;
		for (auto it = data.begin(), end = data.end(); it != end; ++it) {
			if (const auto uploaded = std::get<QDateTime>(it->uploaded()); uploaded > lasySync)
				lasySync = uploaded;
			Q_EMIT downloadedData({type, it.key(), std::get<QJsonObject>(it->data()), it->modified()});
		}
		// if last synced changed -> update it
		if (lasySync > since)
			Q_EMIT updateLastSync(lasySync);
		// if as much data as possible by limit, fetch more with new last sync
		if (data.size() == _limit)
			getChanges(type, lasySync);
		else
			Q_EMIT syncDone(type);
	})->onAllErrors(this, [this](const QString &, int, QtRestClient::RestReply::Error){
		Q_EMIT networkError(tr("Failed to download latests changed data"));
	});
}

void RemoteConnector::uploadChange(const CloudData &data)
{
	auto reply = _api->getData(data.key().typeString(), data.key().id);
	reply->onSucceeded(this, [this, data, reply](int, const std::optional<Data> &replyData) {
		if (!replyData) {
			// no data on the server -> upload with null tag
			doUpload(data, NullETag);
		} else if (replyData->modified() >= data.modified()) {
			// data was modified on the server after modified locally -> ignore upload
			qCDebug(logRmc) << "Server data is newer than local data when trying to upload - triggering sync";
			Q_EMIT downloadedData({data.key(), std::get<QJsonObject>(replyData->data()), replyData->modified()});
			Q_EMIT triggerSync();
		} else {
			// data on server is older -> upload
			doUpload(data, reply->networkReply()->rawHeader("ETag"));
		}
	})->onAllErrors(this, [this, data](const QString &, int, QtRestClient::RestReply::Error) {
		Q_EMIT networkError(tr("Failed to verify data version before uploading"));
	});
}

void RemoteConnector::removeTable(const QByteArray &type)
{
	_api->removeTable(QString::fromUtf8(type))->onSucceeded(this, [this, type](int) {
		Q_EMIT removedTable(type);
	})->onAllErrors(this, [this](const QString &, int, QtRestClient::RestReply::Error) {
		Q_EMIT networkError(tr("Failed to remove table from server"));
	});
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
		const auto line = _eventStream->readLine();
		qCDebug(logRmc) << line;
		// TODO process
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
		qCInfo(logRmc) << "Reconnecting event stream to continue live sync";
		_eventStream->deleteLater();
		_eventStream = nullptr;
		startLiveSync();
		Q_EMIT triggerSync();  // trigger a sync as getChanges is required to unblock the live sync
		break;
	// stopped by user -> live sync stopped
	case QNetworkReply::OperationCanceledError:
		_eventStream->deleteLater();
		_eventStream = nullptr;
		break;
	// inacceptable error codes that indicate a hard failure
	default:
		_eventStream->deleteLater();
		_eventStream = nullptr;
		Q_EMIT networkError(tr("Live-synchronization disabled because of network error!"));
		break;
	}
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
	// data on server is older -> upload
	FirebaseApiBase::ETagSetter eTagSetter{_api, std::move(eTag)};
	_api->uploadData({data.data(), data.modified(), ServerTimestamp{}},
					 data.key().typeString(),
					 data.key().id)
		->onSucceeded(this, [this, key = data.key()](int) {
			Q_EMIT uploadedData(key);
		})->onAllErrors(this, [this](const QString &, int code, QtRestClient::RestReply::Error type){
			if (type == QtRestClient::RestReply::Error::Failure && code == CodeETagMismatch) {
				// data was changed on the server since checked -> trigger sync
				qCDebug(logRmc) << "Server data was changed since the ETag was requested - triggering sync";
				Q_EMIT triggerSync();
			} else
				Q_EMIT networkError(tr("Failed to upload data"));
		});
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
	return {QCborValue::Integer};
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
