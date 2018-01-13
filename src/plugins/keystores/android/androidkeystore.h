#ifndef ANDROIDKEYSTORE_H
#define ANDROIDKEYSTORE_H

#include <QtDataSync/keystore.h>

#include <QtAndroidExtras/QAndroidJniObject>
#include <QtAndroidExtras/QAndroidJniEnvironment>

class AndroidKeyStore : public QtDataSync::KeyStore
{
	Q_OBJECT

public:
	explicit AndroidKeyStore(const QtDataSync::Defaults &defaults, QObject *parent = nullptr);

	QString providerName() const override;
	bool isOpen() const override;
	void openStore() override;
	void closeStore() override;
	bool contains(const QString &key) const override;
	void save(const QString &key, const QByteArray &pKey) override;
	QByteArray load(const QString &key) override;
	void remove(const QString &key) override;

private:
	QAndroidJniObject _preferences;

	void jniThrow(QAndroidJniEnvironment &env) const;
	QAndroidJniObject prefEditor(QAndroidJniEnvironment &env);
	void prefApply(QAndroidJniObject &editor, QAndroidJniEnvironment &env);
};

#endif // ANDROIDKEYSTORE_H
