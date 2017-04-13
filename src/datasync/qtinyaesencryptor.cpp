#include "qtinyaesencryptor_p.h"

#include <QtCore/QJsonDocument>

#include <qtinyaes.h>
using namespace QtDataSync;

QTinyAesEncryptor::QTinyAesEncryptor(QObject *parent) :
	Encryptor(parent),
	key()
{}

void QTinyAesEncryptor::initialize(Defaults *defaults)
{
	key = defaults->property("encryptKey").toByteArray();
	if(key.isNull()) {
		for(auto i = 0; i < QTinyAes::KEYSIZES.first(); i++)
			key += (char)qrand();
	}
}

QJsonValue QTinyAesEncryptor::encrypt(const ObjectKey &key, const QJsonObject &object, const QByteArray &keyProperty) const
{
	auto data = QJsonDocument(object).toBinaryData();
	auto iv = "";
}

QJsonObject QTinyAesEncryptor::decrypt(const ObjectKey &key, const QJsonValue &data, const QByteArray &keyProperty) const
{

}
