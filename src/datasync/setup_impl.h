#ifndef QTDATASYNC_SETUP_IMPL_H
#define QTDATASYNC_SETUP_IMPL_H

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/authenticator.h"

#include <QtCore/qdir.h>
#include <QtCore/qhash.h>
#include <QtCore/qsettings.h>
#include <QtCore/qurl.h>
#include <QtCore/qloggingcategory.h>

class QNetworkAccessManager;
class QRemoteObjectHostBase;

namespace QtDataSync {

class ICloudTransformer;
class Engine;
class EngineThread;

template <typename TAuthenticator, typename TCloudTransformer>
class Setup;

enum class ConfigType {
	WebConfig,
	GoogleServicesJson,
	GoogleServicesPlist
};

namespace __private {

class SetupPrivate;

class Q_DATASYNC_EXPORT SetupExtensionPrivate
{
	Q_DISABLE_COPY(SetupExtensionPrivate)
public:
	SetupExtensionPrivate();
	SetupExtensionPrivate(SetupExtensionPrivate &&other) noexcept;
	SetupExtensionPrivate &operator=(SetupExtensionPrivate &&other) noexcept;
	virtual ~SetupExtensionPrivate();

	virtual void extendFromWebConfig(const QJsonObject &config);
	virtual void extendFromGSJsonConfig(const QJsonObject &config);
	virtual void extendFromGSPlistConfig(QSettings *config);
	virtual void testConfigSatisfied(const QLoggingCategory &logCat);

	virtual QObject *createInstance(QObject *parent) = 0;
};

class Q_DATASYNC_EXPORT SetupPrivate
{
	template <typename TAuthenticator, typename TCloudTransformer>
	friend class QtDataSync::Setup;
	friend class QtDataSync::Engine;

public:
	using ThreadInitFunc = std::function<void(Engine*, QThread*)>;

	struct FirebaseConfig {
		QString projectId;
		QString apiKey;
		std::chrono::milliseconds readTimeOut {std::chrono::minutes{15}};
		int readLimit = 100;
	} firebase;

#ifndef QTDATASYNC_NO_NTP
	QString ntpAddress;
	quint16 ntpPort = 123;
#endif

	QSettings *settings = nullptr;
	QNetworkAccessManager *nam = nullptr;
	QUrl roUrl;
	QRemoteObjectHostBase *roNode = nullptr;

private:
	SetupExtensionPrivate *_authExt;
	SetupExtensionPrivate *_transExt;

	void init(SetupExtensionPrivate *authExt,
			  SetupExtensionPrivate *transExt);

	void readWebConfig(QIODevice *device);
	void readGSJsonConfig(QIODevice *device);
	void readGSPlistConfig(QIODevice *device);
	void testConfigRes();

	static Engine *createEngine(QScopedPointer<SetupPrivate> &&self, QObject *parent);
	static EngineThread *createThreadedEngine(QScopedPointer<SetupPrivate> &&self, ThreadInitFunc &&initFn, QObject *parent);
	void finializeForEngine(Engine *engine);
	IAuthenticator *createAuthenticator(QObject *parent);
	ICloudTransformer *createTransformer(QObject *parent);
};

template <typename TAuthenticator>
class DefaultAuthExtensionPrivate final : public SetupExtensionPrivate
{
public:
	inline QObject *createInstance(QObject *parent) final {
		return new TAuthenticator{parent};
	}
};

template <typename TCloudTransformer>
class DefaultTransformerExtensionPrivate final : public SetupExtensionPrivate
{
public:
	inline QObject *createInstance(QObject *parent) final {
		return new TCloudTransformer{parent};
	}
};

template <typename... TAuthenticators>
class AuthenticationSelectorPrivate final : public SetupExtensionPrivate
{
public:
	QHash<int, SetupExtensionPrivate*> selectDs;

	inline void extendFromWebConfig(const QJsonObject &config) final {
		for (auto d : qAsConst(selectDs))
			d->extendFromWebConfig(config);
	}

	inline void extendFromGSJsonConfig(const QJsonObject &config) final {
		for (auto d : qAsConst(selectDs))
			d->extendFromGSJsonConfig(config);
	}

	inline void extendFromGSPlistConfig(QSettings *config) final {
		for (auto d : qAsConst(selectDs))
			d->extendFromGSPlistConfig(config);
	}

	inline QObject *createInstance(QObject *parent) final {
		auto selector = new AuthenticationSelector<TAuthenticators...>{parent};
		for (auto it = selectDs.constBegin(), end = selectDs.constEnd(); it != end; ++it)
			selector->addAuthenticator(it.key(), it.value()->createInstance(selector));
		return selector;
	}
};

}

Q_DECLARE_LOGGING_CATEGORY(logSetup)

}

#endif // QTDATASYNC_SETUP_IMPL_H
