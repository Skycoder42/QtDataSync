#include "qtinyaesencryptor_p.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/qcryptographichash.h>

#include <qtinyaes.h>
#include <qrng.h>
using namespace QtDataSync;

QTinyAesEncryptor::QTinyAesEncryptor(QObject *parent) :
	Encryptor(parent),
	_defaults(nullptr),
	_key(),
	_keyMutex()
{}

void QTinyAesEncryptor::initialize(Defaults *defaults)
{
	_defaults = defaults;
	_key = _defaults->settings()->value(QStringLiteral("encryption/key")).toByteArray();
	if(_key.isEmpty()) {
		QRng secureRng;
		secureRng.setSecurityLevel(QRng::HighSecurity);
		_key = secureRng.generateRandom(QTinyAes::KEYSIZE);
		_defaults->settings()->setValue(QStringLiteral("encryption/key"), _key);
	}
}

QByteArray QTinyAesEncryptor::key() const
{
	QMutexLocker _(&_keyMutex);
	return _key;
}

void QTinyAesEncryptor::setKey(const QByteArray &key)
{
	QMutexLocker _(&_keyMutex);
	if((quint32)key.size() != QTinyAes::KEYSIZE)
		throw InvalidKeyException();
	_key = key;
	_defaults->settings()->setValue(QStringLiteral("encryption/key"), _key);
}

QJsonValue QTinyAesEncryptor::encrypt(const ObjectKey &key, const QJsonObject &object, const QByteArray &keyProperty) const
{
	//TODO adjust to AES256
	auto salt = QRng().generateRandom(28);//224 bits
	auto iv = QCryptographicHash::hash(salt + key.first + key.second.toUtf8() + keyProperty, QCryptographicHash::Sha3_224);
	iv.resize(QTinyAes::BLOCKSIZE);

	auto data = QJsonDocument(object).toBinaryData();
	auto cipher = QTinyAes::cbcEncrypt( _key, iv, data);

	QJsonObject result;
	result[QStringLiteral("salt")] = QString::fromUtf8(salt.toBase64());
	result[QStringLiteral("data")] = QString::fromUtf8(cipher.toBase64());
	return result;
}

QJsonObject QTinyAesEncryptor::decrypt(const ObjectKey &key, const QJsonValue &data, const QByteArray &keyProperty) const
{
	auto obj = data.toObject();
	auto salt = QByteArray::fromBase64(obj[QStringLiteral("salt")].toString().toUtf8());
	if(salt.size() != 28)//224 bits
		throw DecryptionFailedException();
	auto iv = QCryptographicHash::hash(salt + key.first + key.second.toUtf8() + keyProperty, QCryptographicHash::Sha3_224);
	iv.resize(QTinyAes::BLOCKSIZE);

	auto cipher = QByteArray::fromBase64(obj[QStringLiteral("data")].toString().toUtf8());
	if(cipher.size() % QTinyAes::KEYSIZE != 0)
		throw DecryptionFailedException();

	auto plain = QTinyAes::cbcDecrypt(_key, iv, cipher);
	auto json = QJsonDocument::fromBinaryData(plain);
	if(json.isObject())
		return json.object();
	else
		throw DecryptionFailedException();
}



const char *InvalidKeyException::what() const noexcept
{
	return "The given key does not have the valid length of 128 bit!";
}

void InvalidKeyException::raise() const
{
	throw *this;
}

QException *InvalidKeyException::clone() const
{
	return new InvalidKeyException();
}

const char *DecryptionFailedException::what() const noexcept
{
	return "Failed to decrypt data returned from server. Maybe it's not encrypted?";
}

void DecryptionFailedException::raise() const
{
	throw *this;
}

QException *DecryptionFailedException::clone() const
{
	return new DecryptionFailedException();
}
