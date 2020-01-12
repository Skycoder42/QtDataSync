#ifndef QTDATASYNC_SETUP_H
#define QTDATASYNC_SETUP_H

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/engine.h"
#include "QtDataSync/exception.h"
#include "QtDataSync/setup_impl.h"

#include <chrono>

#include <QtCore/qobject.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qsettings.h>

namespace QtDataSync {

class OAuthAuthenticator;

// basic extenders for any non specialized setup
template <typename TSetup, typename TAuthenticator>
class SetupAuthenticationExtension
{
protected:
	void extendAuthFromConfig(const QJsonObject &config);
#ifdef Q_OS_DARWIN
	void extendAuthFromConfig(QSettings *config);
#endif

	FirebaseAuthenticator *createAuthenticator(const QString &apiKey, QSettings *settings);
};

template <typename TSetup, typename TCloudTransformer>
class SetupTransformerExtension
{
protected:
	void extendTransformFromConfig(const QJsonObject &config);
#ifdef Q_OS_DARWIN
	void extendTransformFromConfig(QSettings *config);
#endif

	ICloudTransformer *createTransformer();
};

class SetupPrivate;
template <typename TAuthenticator, typename TCloudTransformer>
class Setup :
  public SetupAuthenticationExtension<Setup<TAuthenticator, TCloudTransformer>, TAuthenticator>,
  public SetupTransformerExtension<Setup<TAuthenticator, TCloudTransformer>, TCloudTransformer>
{
	Q_DISABLE_COPY(Setup)

public:
	using ConfigType = SetupPrivate::ConfigType;
	using AuthExtension = SetupAuthenticationExtension<Setup<TAuthenticator, TCloudTransformer>, TAuthenticator>;
	using TransformExtension = SetupTransformerExtension<Setup<TAuthenticator, TCloudTransformer>, TCloudTransformer>;

	inline Setup() :
		d{new SetupPrivate{}}
	{}
	inline Setup(Setup &&other) noexcept {
		d.swap(other.d);
	}
	inline Setup &operator=(Setup &&other) noexcept {
		d.swap(other.d);
		return *this;
	}
	inline ~Setup() = default;

	inline void swap(Setup &other) {
		d.swap(other.d);
	}

	static Setup fromConfig(const QString &configPath, ConfigType configType);
	static Setup fromConfig(QIODevice *configDevice, ConfigType configType);

	inline Setup &setSettings(QSettings *settings) {
		d->settings = settings;
		return *this;
	}

	inline Setup &setFirebaseProjectId(QString projectId) {
		d->projectId = std::move(projectId);
		return *this;
	}

	inline Setup &setFirebaseWebApiKey(QString webApiKey) {
		d->apiKey = std::move(webApiKey);
		return *this;
	}

	inline Setup &setRemoteReadTimeout(std::chrono::milliseconds timeout) {
		d->readTimeOut = std::move(timeout);
		return *this;
	}

	inline Setup &setRemotePageLimit(int limit) {
		d->readLimit = limit;
		return *this;
	}

#ifndef QTDATASYNC_NO_NTP
	inline Setup &enableNtpSync(QString hostName = QStringLiteral("pool.ntp.org"), quint16 port = 123) {
		d->ntpAddress = std::move(hostName);
		d->ntpPort = port;
		return *this;
	}
#endif

	inline Engine *createEngine(QObject *parent = nullptr) {
		return d->createEngine(AuthExtension::createAuthenticator(d->apiKey, d->settings),
							   TransformExtension::createTransformer(),
							   parent);
	}

	inline friend Q_DATASYNC_EXPORT void swap(Setup& lhs, Setup& rhs) {
		lhs.swap(rhs);
	}

private:
	QScopedPointer<SetupPrivate> d;
};

template <typename TSetup>
class SetupAuthenticationExtension<TSetup, OAuthAuthenticator>
{
public:
	inline TSetup &setOAuthClientId(QString clientId) {
		d->clientId = std::move(clientId);
		return *static_cast<TSetup>(this);
	}

	inline TSetup &setOAuthClientSecret(QString secret) {
		d->secret = std::move(secret);
		return *static_cast<TSetup>(this);
	}

	inline TSetup &setOAuthClientCallbackPort(quint16 port) {
		d->port = port;
		return *static_cast<TSetup>(this);
	}

protected:
	void extendAuthFromConfig(const QJsonObject &config);
#ifdef Q_OS_DARWIN
	void extendAuthFromConfig(QSettings *config);
#endif

	FirebaseAuthenticator *createAuthenticator(const QString &apiKey, QSettings *settings) {
		return nullptr; // TODO create -> move to seperate header?
	}

private:
	QScopedPointer<OAuthExtenderPrivate> d;
};

template<typename TAuthenticator, typename TCloudTransformer>
Setup<TAuthenticator, TCloudTransformer> Setup<TAuthenticator, TCloudTransformer>::fromConfig(const QString &configPath, Setup::ConfigType configType)
{
	QFile configFile{configPath};
	if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text))
		throw FileException{&configFile};
	return fromConfig(&configFile, configType);
}

template<typename TAuthenticator, typename TCloudTransformer>
Setup<TAuthenticator, TCloudTransformer> Setup<TAuthenticator, TCloudTransformer>::fromConfig(QIODevice *configDevice, ConfigType configType)
{
	Setup<TAuthenticator, TCloudTransformer> setup;
	switch (configType) {
	case ConfigType::WebConfig:
		setup.readWebConfig(configDevice, [&](const QJsonObject &config){
			setup.extendAuthFromConfig(config);
			setup.extendTransformFromConfig(config);
		});
		break;
	case ConfigType::GoogleServicesJson:
		setup.readGSJsonConfig(configDevice, [&](const QJsonObject &config){
			setup.extendAuthFromConfig(config);
			setup.extendTransformFromConfig(config);
		});
		break;
	case ConfigType::GoogleServicesPlist:
		setup.readGSPlistConfig(configDevice, [&](QSettings *config){
			setup.extendAuthFromConfig(config);
			setup.extendTransformFromConfig(config);
		});
		break;
	}
	return setup;
}

class Q_DATASYNC_EXPORT SetupException : public Exception
{
public:
	SetupException(QString error);

	QString qWhat() const override;

	void raise() const override;
	ExceptionBase *clone() const override;

protected:
	QString _error;
};

}

#endif // QTDATASYNC_SETUP_H
