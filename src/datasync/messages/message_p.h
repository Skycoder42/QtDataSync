#ifndef QTDATASYNC_MESSAGE_P_H
#define QTDATASYNC_MESSAGE_P_H

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

class Q_DATASYNC_EXPORT Message
{
	Q_GADGET

public:
	static const QByteArray PingMessage;

	static void registerTypes();

	virtual ~Message();

	const QMetaObject *metaObject() const;

	QByteArray messageName() const;
	template <typename TMessage>
	inline static QByteArray messageName();

	QByteArray typeName() const;
	static QByteArray typeName(const QByteArray &messageName);

	template <typename TMessage>
	static inline bool isType(const QByteArray &name);

	void serializeTo(QDataStream &stream, bool withName = true) const;
	QByteArray serialize() const;
	QByteArray serializeSigned(const CryptoPP::PKCS8PrivateKey &key,
							   CryptoPP::RandomNumberGenerator &rng,
							   AsymmetricCrypto *crypto) const;

	static void setupStream(QDataStream &stream);
	static void deserializeMessageTo(QDataStream &stream, Message &message);
	template <typename TMessage>
	static inline TMessage deserializeMessage(QDataStream &stream);
	static void verifySignature(QDataStream &stream, const CryptoPP::X509PublicKey &key, AsymmetricCrypto *crypto);
	static inline void verifySignature(QDataStream &stream, const QSharedPointer<CryptoPP::X509PublicKey> &key, AsymmetricCrypto *crypto) {
		return verifySignature(stream, *key, crypto);
	}

protected:
	virtual const QMetaObject *getMetaObject() const = 0;
	virtual bool validate();

private:
	static QByteArray msgNameImpl(const QMetaObject *getMetaObject);
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const Message &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, Message &message);

// ------------- Generic Implementation -------------

template<typename TMessage>
inline QByteArray Message::messageName()
{
	static_assert(std::is_void<typename TMessage::QtGadgetHelper>::value, "Only Q_GADGETS can be serialized");
	return msgNameImpl(&TMessage::staticMetaObject);
}

template<typename TMessage>
inline bool Message::isType(const QByteArray &name)
{
	static_assert(std::is_void<typename TMessage::QtGadgetHelper>::value, "Only Q_GADGETS can be serialized");
	return (messageName<TMessage>() == name);
}

template <typename TMessage>
inline TMessage Message::deserializeMessage(QDataStream &stream)
{
	static_assert(std::is_void<typename TMessage::QtGadgetHelper>::value, "Only Q_GADGETS can be serialized");
	TMessage message;
	deserializeMessageTo(stream, message);
	return message;
}

}

Q_DECLARE_METATYPE(QtDataSync::Utf8String)

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

#endif // QTDATASYNC_MESSAGE_H
