#include "googleoauthflow_p.h"

#include <QtCore/QCryptographicHash>
#include <QtNetwork/QNetworkReply>
using namespace QtDataSync;

GoogleOAuthFlow::GoogleOAuthFlow(quint16 port, QNetworkAccessManager *nam, QObject *parent) :
	QOAuth2AuthorizationCodeFlow{nam, parent}
{
	_handler = new GoogleOAuthHandler{port, this};
	connect(_handler, &GoogleOAuthHandler::tokensReceived,
			this, &GoogleOAuthFlow::handleIdToken);
	connect(_handler, &GoogleOAuthHandler::apiError,
			this, &GoogleOAuthFlow::error);
	setReplyHandler(_handler);

	setAuthorizationUrl(QStringLiteral("https://accounts.google.com/o/oauth2/v2/auth"));
	setAccessTokenUrl(QStringLiteral("https://www.googleapis.com/oauth2/v4/token"));
	setModifyParametersFunction([this](QAbstractOAuth::Stage stage, QVariantMap *parameters) {
		switch (stage) {
		case QAbstractOAuth::Stage::RequestingAuthorization:
			// add parameters
			parameters->insert(QStringLiteral("prompt"), QStringLiteral("select_account"));
			parameters->insert(QStringLiteral("access_type"), QStringLiteral("offline"));
			parameters->insert(QStringLiteral("code_challenge_method"), QStringLiteral("S256"));
			parameters->insert(QStringLiteral("code_challenge"), createChallenge());
			break;
		case QAbstractOAuth::Stage::RequestingAccessToken:
			parameters->insert(QStringLiteral("code_verifier"), _codeVerifier);
			urlUnescape(parameters, QStringLiteral("code"));
			urlUnescape(parameters, QStringLiteral("redirect_uri"));
			break;
		case QAbstractOAuth::Stage::RefreshingAccessToken:
			parameters->insert(QStringLiteral("client_id"), clientIdentifier());
			parameters->insert(QStringLiteral("client_secret"), clientIdentifierSharedKey());
			urlUnescape(parameters, QStringLiteral("redirect_uri"));
			break;
		default:
			break;
		}
	});
}

QString GoogleOAuthFlow::idToken() const
{
	return _idToken;
}

QUrl GoogleOAuthFlow::requestUrl() const
{
	return _handler->callback();
}

void GoogleOAuthFlow::setIdToken(QString idToken)
{
	_idToken = std::move(idToken);
}

void GoogleOAuthFlow::handleIdToken(const QVariantMap &values)
{
	_idToken = values.value(QStringLiteral("id_token")).toString();
}

QString GoogleOAuthFlow::createChallenge()
{
	if (!_challengeEngine)
		_challengeEngine = BitEngine{QRandomGenerator::securelySeeded()};

	QByteArray data(96, 0);
	std::generate(data.begin(), data.end(), *_challengeEngine);
	const auto verifierData = data.toBase64(QByteArray::Base64UrlEncoding);
	_codeVerifier = QString::fromUtf8(verifierData);

	return QString::fromUtf8(
		QCryptographicHash::hash(verifierData, QCryptographicHash::Sha256)
			.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

void GoogleOAuthFlow::urlUnescape(QVariantMap *parameters, const QString &key) const
{
	parameters->insert(key, QUrl::fromPercentEncoding(parameters->value(key).toString().toUtf8()));
}



GoogleOAuthHandler::GoogleOAuthHandler(quint16 port, QObject *parent) :
	QOAuthHttpServerReplyHandler{port, parent}
{}

void GoogleOAuthHandler::networkReplyFinished(QNetworkReply *reply)
{
	QOAuthHttpServerReplyHandler::networkReplyFinished(reply);
	if (!reply->atEnd()) {
		const auto errData = reply->readAll();
		const auto errDoc = QJsonDocument::fromJson(errData);
		if (errDoc.isObject()) {
			const auto errObj = errDoc.object();
			emit apiError(errObj[QStringLiteral("error")].toString(),
						  errObj[QStringLiteral("error_description")].toString(),
						  errObj[QStringLiteral("error_uri")].toString());
		} else {
			emit apiError(QStringLiteral("Network-Request failed"), QString::fromUtf8(errData.simplified()));
		}
	}
}
