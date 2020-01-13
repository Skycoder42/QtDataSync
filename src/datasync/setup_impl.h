#ifndef QTDATASYNC_SETUP_IMPL_H
#define QTDATASYNC_SETUP_IMPL_H

#include "QtDataSync/qtdatasync_global.h"

#include <QtCore/qdir.h>
#include <QtCore/qsettings.h>
#include <QtCore/qloggingcategory.h>

namespace QtDataSync {

class FirebaseAuthenticator;
class ICloudTransformer;
class Engine;

template <typename TAuthenticator, typename TCloudTransformer>
class Setup;

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

	virtual void extendFromWebConfig(const QJsonObject &config);;
	virtual void extendFromGSJsonConfig(const QJsonObject &config);;
	virtual void extendFromGSPlistConfig(QSettings *config);;

	virtual QObject *createInstance(const SetupPrivate &d, QObject *parent) = 0;
};

class Q_DATASYNC_EXPORT SetupPrivate
{
	template <typename TAuthenticator, typename TCloudTransformer>
	friend class QtDataSync::Setup;
	friend class QtDataSync::Engine;

public:
	enum class ConfigType {  // TODO move to namespace
		WebConfig,
		GoogleServicesJson,
		GoogleServicesPlist
	};

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

private:
	SetupExtensionPrivate *_authExt;
	SetupExtensionPrivate *_transExt;

	void init(SetupExtensionPrivate *authExt,
			  SetupExtensionPrivate *transExt);

	void readWebConfig(QIODevice *device);
	void readGSJsonConfig(QIODevice *device);
	void readGSPlistConfig(QIODevice *device);

	static Engine *createEngine(QScopedPointer<SetupPrivate> &&self, QObject *parent);
	void finializeForEngine(Engine *engine);
	FirebaseAuthenticator *createAuthenticator(QObject *parent);
	ICloudTransformer *createTransformer(QObject *parent);
};

template <typename TAuthenticator>
class DefaultAuthExtensionPrivate final : public SetupExtensionPrivate
{
public:
	inline QObject *createInstance(const SetupPrivate &d, QObject *parent) final {
		return new TAuthenticator{d.firebase.apiKey, d.settings, parent};
	}
};

template <typename TCloudTransformer>
class DefaultTransformerExtensionPrivate final : public SetupExtensionPrivate
{
public:
	inline QObject *createInstance(const SetupPrivate &, QObject *parent) final {
		return new TCloudTransformer{parent};
	}
};

class OAuthExtensionPrivate final : public SetupExtensionPrivate
{
public:
	QString clientId;
	QString secret;
	quint16 port = 0;

	void extendFromWebConfig(const QJsonObject &config) override;
	void extendFromGSJsonConfig(const QJsonObject &config) override;
	void extendFromGSPlistConfig(QSettings *config) override;
	QObject *createInstance(const SetupPrivate &d, QObject *parent) final;
};

}

Q_DECLARE_LOGGING_CATEGORY(logSetup)

}

#endif // QTDATASYNC_SETUP_IMPL_H
