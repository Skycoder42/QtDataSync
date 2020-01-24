#ifndef QTDATASYNC_SETUP_H
#define QTDATASYNC_SETUP_H

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/setup_impl.h"
#include "QtDataSync/exception.h"
#include "QtDataSync/authenticator.h"
#include "QtDataSync/cloudtransformer.h"

#include <chrono>

#include <QtCore/qobject.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qsettings.h>

namespace QtDataSync {

// basic extenders for any non specialized setup
template <typename TSetup, typename TAuthenticator>
class SetupAuthenticationExtension
{
protected:
	inline __private::SetupExtensionPrivate *authenticatorD() {
		return &d;
	}

private:
	__private::DefaultAuthExtensionPrivate<TAuthenticator> d;
};

template <typename TSetup, typename TCloudTransformer>
class SetupTransformerExtension
{
protected:
	inline __private::SetupExtensionPrivate *transformD() {
		return &d;
	}

private:
	__private::DefaultTransformerExtensionPrivate<TCloudTransformer> d;
};


template <typename TAuthenticator, typename TCloudTransformer = PlainCloudTransformer>
class Setup :
  public SetupAuthenticationExtension<Setup<TAuthenticator, TCloudTransformer>, TAuthenticator>,
  public SetupTransformerExtension<Setup<TAuthenticator, TCloudTransformer>, TCloudTransformer>
{
	static_assert (std::is_base_of_v<IAuthenticator, TAuthenticator>, "TAuthenticator must extend IAuthenticator or AuthenticationSelector<...>");
	static_assert (std::is_base_of_v<ICloudTransformer, TCloudTransformer>, "TCloudTransformer must implement ICloudTransformer");
	Q_DISABLE_COPY(Setup)

public:
	using AuthExtension = SetupAuthenticationExtension<Setup<TAuthenticator, TCloudTransformer>, TAuthenticator>;
	using TransformExtension = SetupTransformerExtension<Setup<TAuthenticator, TCloudTransformer>, TCloudTransformer>;
	using ThreadInitFunc = __private::SetupPrivate::ThreadInitFunc;

	inline Setup() :
		d{new __private::SetupPrivate{}}
	{
		d->init(this->authenticatorD(), this->transformD());
	}

	inline Setup(Setup &&other) noexcept {
		d.swap(other.d);
	}

	inline Setup &operator=(Setup &&other) noexcept {
		d.swap(other.d);
		return *this;
	}

	inline ~Setup() = default;

	static Setup fromConfig(const QString &configPath, ConfigType configType);
	static Setup fromConfig(QIODevice *configDevice, ConfigType configType);

	inline Setup &setFirebaseProjectId(QString projectId) {
		d->firebase.projectId = std::move(projectId);
		return *this;
	}

	inline Setup &setFirebaseWebApiKey(QString webApiKey) {
		d->firebase.apiKey = std::move(webApiKey);
		return *this;
	}

	inline Setup &setRemoteReadTimeout(std::chrono::milliseconds timeout) {
		d->firebase.readTimeOut = std::move(timeout);
		return *this;
	}

	inline Setup &setRemotePageLimit(int limit) {
		d->firebase.readLimit = limit;
		return *this;
	}

	inline Setup &setSettings(QSettings *settings) {
		d->settings = settings;
		return *this;
	}

	inline Setup &setNetworkAccessManager(QNetworkAccessManager *nam) {
		d->nam = nam;
		return *this;
	}

	inline Setup &setAsyncUrl(QUrl url) {
		d->roUrl = std::move(url);
		d->roNode = nullptr;
		return *this;
	}

	inline Setup &setAsyncHost(QRemoteObjectHostBase *node) {
		d->roUrl.clear();
		d->roNode = node;
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
		return __private::SetupPrivate::createEngine(std::move(d), parent);
	}

	inline EngineThread *createThreadedEngine(ThreadInitFunc initFn, QObject *parent = nullptr) {
		return __private::SetupPrivate::createThreadedEngine(std::move(d), std::move(initFn), parent);
	}

private:
	friend class __private::SetupPrivate;
	QScopedPointer<__private::SetupPrivate> d;
};

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

// ------------- implementation -------------

template <typename TSetup, typename... TAuthenticators>
class SetupAuthenticationExtension<TSetup, AuthenticationSelector<TAuthenticators...>> : public SetupAuthenticationExtension<TSetup, TAuthenticators>...
{
	Q_DISABLE_COPY(SetupAuthenticationExtension)
public:
	inline SetupAuthenticationExtension() :
		d{
			{std::make_pair(qMetaTypeId<TAuthenticators>(), SetupAuthenticationExtension<TSetup, TAuthenticators>::authenticatorD())...}
		}
	{}
	inline SetupAuthenticationExtension(SetupAuthenticationExtension &&) noexcept = default;
	inline SetupAuthenticationExtension &operator=(SetupAuthenticationExtension &&) noexcept = default;

	template <typename TAuth>
	inline SetupAuthenticationExtension<TSetup, TAuth> &authExtension() {
		static_assert (std::is_base_of_v<SetupAuthenticationExtension<TSetup, TAuth>, SetupAuthenticationExtension<TSetup, AuthenticationSelector<TAuthenticators...>>>, "TAuth must be one of the specified authenticators");
		return *this;
	}

protected:
	inline __private::SetupExtensionPrivate *authenticatorD() {
		return &d;
	}

private:
	__private::AuthenticationSelectorPrivate<TAuthenticators...> d;
};

template<typename TAuthenticator, typename TCloudTransformer>
Setup<TAuthenticator, TCloudTransformer> Setup<TAuthenticator, TCloudTransformer>::fromConfig(const QString &configPath, ConfigType configType)
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
		setup.d->readWebConfig(configDevice);
		break;
	case ConfigType::GoogleServicesJson:
		setup.d->readGSJsonConfig(configDevice);
		break;
	case ConfigType::GoogleServicesPlist:
		setup.d->readGSPlistConfig(configDevice);
		break;
	}
	return setup;
}

}

#endif // QTDATASYNC_SETUP_H
