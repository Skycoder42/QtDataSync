#ifndef EXTENSIONS_H
#define EXTENSIONS_H

#include <QtDataSync/Setup>

class DummyAuth : public QtDataSync::IAuthenticator
{
	Q_OBJECT

public:
	DummyAuth(int value, QObject *parent);

	int value;

protected:
	void signIn() override;
	void logOut() override;
	void abortRequest() override;
};

Q_DECLARE_METATYPE(DummyAuth*)

class DummyTransform : public QtDataSync::ICloudTransformer
{
	Q_OBJECT

public:
	DummyTransform(int value, QObject *parent);

	int value;

public Q_SLOTS:
	void transformUpload(const QtDataSync::LocalData &data) override;
	void transformDownload(const QtDataSync::CloudData &data) override;
};



template <typename TDummy>
class DummyExtensionPrivate : public QtDataSync::__private::SetupExtensionPrivate
{
public:
	bool invalid = false;
	int value = 0;

	void extendFromWebConfig(const QJsonObject &config) override {
		const auto key = QString::fromUtf8(TDummy::staticMetaObject.className());
		if (config.contains(key))
			value = config.value(key).toInt() + 0;
	}

	void extendFromGSJsonConfig(const QJsonObject &config) override {
		const auto key = QString::fromUtf8(TDummy::staticMetaObject.className());
		if (config.contains(key))
			value = config.value(key).toInt() + 1;
	}

	void extendFromGSPlistConfig(QSettings *config) override {
		const auto key = QString::fromUtf8(TDummy::staticMetaObject.className());
		if (config->contains(key))
			value = config->value(key).toInt() + 2;
	}

	void testConfigSatisfied(const QLoggingCategory &logCat) override {
		if (value == 0) {
			value = -1;
			qCWarning(logCat) << "Value not set";
		}
	}

	QObject *createInstance(QObject *parent) override {
		return invalid ? nullptr : new TDummy{value, parent};
	}
};



namespace QtDataSync {

template <typename TSetup>
class SetupAuthenticationExtension<TSetup, DummyAuth>
{
public:
	inline TSetup &setInvalid() {
		d->invalid = true;
		return *static_cast<TSetup*>(this);
	}

	inline TSetup &setAuthValue(int value) {
		d->value = value;
		return *static_cast<TSetup*>(this);
	}

protected:
	inline __private::SetupExtensionPrivate *createAuthenticatorD() {
		d = new DummyExtensionPrivate<DummyAuth>{};
		return d;
	}

private:
	DummyExtensionPrivate<DummyAuth> *d = nullptr;
};

template <typename TSetup>
class SetupTransformerExtension<TSetup, DummyTransform>
{
public:
	inline TSetup &setTransformValue(int value) {
		d->value = value;
		return *static_cast<TSetup*>(this);
	}

protected:
	inline __private::SetupExtensionPrivate *createTransformerD() {
		d = new DummyExtensionPrivate<DummyTransform>{};
		return d;
	}

private:
	DummyExtensionPrivate<DummyTransform> *d = nullptr;
};

}

#endif // EXTENSIONS_H
