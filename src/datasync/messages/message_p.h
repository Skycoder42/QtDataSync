#ifndef MESSAGE_P_H
#define MESSAGE_P_H

#include <tuple>

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

class Q_DATASYNC_EXPORT Utf8String : public QString
{
public:
	Utf8String();
	Utf8String(const QByteArray &data);
	Utf8String(const QString &other);
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const Utf8String &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, Utf8String &message);


extern Q_DATASYNC_EXPORT const QByteArray PingMessage;

void Q_DATASYNC_EXPORT setupStream(QDataStream &stream);
void Q_DATASYNC_EXPORT verifySignature(QDataStream &stream, const CryptoPP::X509PublicKey &key, AsymmetricCrypto *crypto);
inline void verifySignature(QDataStream &stream, const QSharedPointer<CryptoPP::X509PublicKey> &key, AsymmetricCrypto *crypto) {
	return verifySignature(stream, *(key.data()), crypto);
}
QByteArray Q_DATASYNC_EXPORT createSignature(const QByteArray &message, const CryptoPP::PKCS8PrivateKey &key, CryptoPP::RandomNumberGenerator &rng, AsymmetricCrypto *crypto);

template <typename TMessage>
QByteArray messageName();
QByteArray Q_DATASYNC_EXPORT typeName(const QByteArray &messageName);

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

template<typename TMessage>
QByteArray messageName()
{
	static_assert(std::is_void<typename TMessage::QtGadgetHelper>::value, "Only Q_GADGETS can be serialized");
	auto name = QByteArray(TMessage::staticMetaObject.className());
	Q_ASSERT_X(name.startsWith("QtDataSync::"), Q_FUNC_INFO, "Message is not in QtDataSync namespace");
	Q_ASSERT_X(name.endsWith("Message"), Q_FUNC_INFO, "Message does not have the Message suffix");
	name = name.mid(12); //strlen("QtDataSync::")
	name.chop(7); //strlen("Message")
	return name;
}

template <typename TMessage>
void serializeMessage(QDataStream &stream, const TMessage &message, bool withName)
{
	static_assert(std::is_void<typename TMessage::QtGadgetHelper>::value, "Only Q_GADGETS can be serialized");
	if(withName)
		stream << messageName<TMessage>();
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
	return (messageName<TMessage>() == name);
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

//define outside of namespace for clang
template<typename... Args>
QDataStream &operator<<(QDataStream &stream, const std::tuple<Args...> &message);
template<typename... Args>
QDataStream &operator>>(QDataStream &stream, std::tuple<Args...> &message);

//helper method
template <size_t index, typename TTuple, typename TCurrent>
void writeTuple(QDataStream &stream, const TTuple &data) {
	stream << std::get<index>(data);
}

//helper method
template <size_t index, typename TTuple, typename TCurrent, typename TNext, typename... TArgs>
void writeTuple(QDataStream &stream, const TTuple &data) {
	stream << std::get<index>(data);
	writeTuple<index+1, TTuple, TNext, TArgs...>(stream, data);
}

template<typename... Args>
QDataStream &operator<<(QDataStream &stream, const std::tuple<Args...> &message) {
	typedef std::tuple<Args...> tpl;
	writeTuple<0, tpl, Args...>(stream, message);
	return stream;
}

//helper method
template <size_t index, typename TTuple, typename TCurrent>
void readTuple(QDataStream &stream, TTuple &data) {
	stream >> std::get<index>(data);
}

//helper method
template <size_t index, typename TTuple, typename TCurrent, typename TNext, typename... TArgs>
void readTuple(QDataStream &stream, TTuple &data) {
	stream >> std::get<index>(data);
	readTuple<index+1, TTuple, TNext, TArgs...>(stream, data);
}

template<typename... Args>
QDataStream &operator>>(QDataStream &stream, std::tuple<Args...> &message) {
	typedef std::tuple<Args...> tpl;
	stream.startTransaction();
	readTuple<0, tpl, Args...>(stream, message);
	stream.commitTransaction();
	return stream;
}

#endif // MESSAGE_H
