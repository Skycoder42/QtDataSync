#ifndef QTDATASYNC_QTINYAESENCRYPTOR_P_H
#define QTDATASYNC_QTINYAESENCRYPTOR_P_H

#include "qtdatasync_global.h"
#include "encryptor.h"

#include <QtCore/QException>

namespace QtDataSync {

class Q_DATASYNC_EXPORT InvalidKeyException : public QException
{
public:
	const char *what() const noexcept override;
	void raise() const override;
	QException *clone() const override;
};

class Q_DATASYNC_EXPORT DecryptionFailedException : public QException
{
public:
	const char *what() const noexcept override;
	void raise() const override;
	QException *clone() const override;
};

class Q_DATASYNC_EXPORT QTinyAesEncryptor : public Encryptor
{
	Q_OBJECT

public:
	explicit QTinyAesEncryptor(QObject *parent = nullptr);

	void initialize(Defaults *defaults) override;

	QByteArray key() const override;
	void setKey(const QByteArray &key) override;

	QJsonValue encrypt(const ObjectKey &key, const QJsonObject &object, const QByteArray &keyProperty) const override;
	QJsonObject decrypt(const ObjectKey &key, const QJsonValue &data, const QByteArray &keyProperty) const override;

private:
	Defaults *_defaults;
	QByteArray _key;
};

}

#endif // QTDATASYNC_QTINYAESENCRYPTOR_P_H
