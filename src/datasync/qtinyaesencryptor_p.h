#ifndef QTDATASYNC_QTINYAESENCRYPTOR_P_H
#define QTDATASYNC_QTINYAESENCRYPTOR_P_H

#include "qtdatasync_global.h"
#include "encryptor.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT QTinyAesEncryptor : public Encryptor
{
	Q_OBJECT

public:
	explicit QTinyAesEncryptor(QObject *parent = nullptr);

	void initialize(Defaults *defaults) override;

	QJsonValue encrypt(const ObjectKey &key, const QJsonObject &object, const QByteArray &keyProperty) const override;
	QJsonObject decrypt(const ObjectKey &key, const QJsonValue &data, const QByteArray &keyProperty) const override;

private:
	QByteArray key;
};

}

#endif // QTDATASYNC_QTINYAESENCRYPTOR_P_H
