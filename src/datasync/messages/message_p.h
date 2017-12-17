#ifndef MESSAGE_P_H
#define MESSAGE_P_H

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QDataStream>
#include <QtCore/QException>
#include <QtCore/QSharedPointer>

#include <cryptopp/rng.h>
#include <cryptopp/asn.h>

#ifdef BUILD_QDATASYNCSERVER
#define Q_DATASYNC_EXPORT
#else
#include "qtdatasync_global.h"
#endif

namespace QtDataSync {

class AsymmetricCrypto;

class Q_DATASYNC_EXPORT DataStreamException : public QException
{
public:
	DataStreamException(QDataStream &stream);

	const char *what() const noexcept override;
	void raise() const override;
	QException *clone() const override;

private:
	DataStreamException(QDataStream::Status status);
	QDataStream::Status _status;
};

void Q_DATASYNC_EXPORT setupStream(QDataStream &stream);
void Q_DATASYNC_EXPORT verifySignature(QDataStream &stream, const CryptoPP::X509PublicKey &key, AsymmetricCrypto *crypto);
inline void verifySignature(QDataStream &stream, const QSharedPointer<CryptoPP::X509PublicKey> &key, AsymmetricCrypto *crypto) {
	return verifySignature(stream, *(key.data()), crypto);
}
QByteArray Q_DATASYNC_EXPORT createSignature(const QByteArray &message, const CryptoPP::PKCS8PrivateKey &key, CryptoPP::RandomNumberGenerator &rng, AsymmetricCrypto *crypto);

template <typename TMessage>
void serializeMessage(QDataStream &stream, const TMessage &message, bool withName = true);
template <typename TMessage>
QByteArray serializeMessage(const TMessage &message);
template <typename TMessage>
QByteArray serializeSignedMessage(const TMessage &message,
								  const CryptoPP::PKCS8PrivateKey &key,
								  CryptoPP::RandomNumberGenerator &rng,
								  AsymmetricCrypto *crypto);

template <typename TMessage>
inline bool isType(const QByteArray &name);
template <typename TMessage>
TMessage deserializeMessage(QDataStream &stream);

// ------------- Generic Implementation -------------

template <typename TMessage>
void serializeMessage(QDataStream &stream, const TMessage &message, bool withName)
{
	static_assert(std::is_void<typename TMessage::QtGadgetHelper>::value, "Only Q_GADGETS can be serialized");
	if(withName)
		stream << QByteArray(TMessage::staticMetaObject.className());
	stream << message;
	if(stream.status() != QDataStream::Ok)
		throw DataStreamException(stream);
}

template <typename TMessage>
QByteArray serializeMessage(const TMessage &message)
{
	static_assert(std::is_void<typename TMessage::QtGadgetHelper>::value, "Only Q_GADGETS can be serialized");
	QByteArray out;
	QDataStream stream(&out, QIODevice::WriteOnly | QIODevice::Unbuffered);
	setupStream(stream);
	serializeMessage(stream, message, true);
	return out;
}

template<typename TMessage>
QByteArray serializeSignedMessage(const TMessage &message, const CryptoPP::PKCS8PrivateKey &key, CryptoPP::RandomNumberGenerator &rng, AsymmetricCrypto *crypto)
{
	static_assert(std::is_void<typename TMessage::QtGadgetHelper>::value, "Only Q_GADGETS can be serialized");
	QByteArray out;
	QDataStream stream(&out, QIODevice::WriteOnly | QIODevice::Unbuffered);
	setupStream(stream);
	serializeMessage(stream, message, true);
	stream << createSignature(out, key, rng, crypto);
	return out;
}

template<typename TMessage>
inline bool isType(const QByteArray &name)
{
	static_assert(std::is_void<typename TMessage::QtGadgetHelper>::value, "Only Q_GADGETS can be serialized");
	return (TMessage::staticMetaObject.className() == name);
}

template <typename TMessage>
TMessage deserializeMessage(QDataStream &stream)
{
	static_assert(std::is_void<typename TMessage::QtGadgetHelper>::value, "Only Q_GADGETS can be serialized");
	stream.startTransaction();
	TMessage message;
	stream >> message;
	if(stream.commitTransaction())
		return message;
	else
		throw DataStreamException(stream);
}

}

#endif // MESSAGE_H
