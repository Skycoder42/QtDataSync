#include "mockencryptor.h"

MockEncryptor::MockEncryptor(QObject *parent) :
	Encryptor(parent),
	dummyKey()
{}

QByteArray MockEncryptor::key() const
{
	return dummyKey;
}

QJsonValue MockEncryptor::encrypt(const QtDataSync::ObjectKey &, const QJsonObject &object, const QByteArray &) const
{
	return object;
}

QJsonObject MockEncryptor::decrypt(const QtDataSync::ObjectKey &, const QJsonValue &data, const QByteArray &) const
{
	return data.toObject();
}

void MockEncryptor::setKey(const QByteArray &key)
{
	dummyKey = key;
}
