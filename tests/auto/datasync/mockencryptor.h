#ifndef MOCKENCRYPTOR_H
#define MOCKENCRYPTOR_H

#include <encryptor.h>

class MockEncryptor : public QtDataSync::Encryptor
{
	Q_OBJECT

public:
	MockEncryptor(QObject *parent = nullptr);

	QByteArray key() const override;
	QJsonValue encrypt(const QtDataSync::ObjectKey &key, const QJsonObject &object, const QByteArray &keyProperty) const override;
	QJsonObject decrypt(const QtDataSync::ObjectKey &key, const QJsonValue &data, const QByteArray &keyProperty) const override;

public slots:
	void setKey(const QByteArray &key) override;

public:
	QByteArray dummyKey;
};

#endif // MOCKENCRYPTOR_H
