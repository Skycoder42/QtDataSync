#include "message_p.h"
#include "asymmetriccrypto_p.h"

#include <QtCore/QMetaProperty>
#include <QtCore/QVersionNumber>

#include "devicesmessage_p.h"
#include "devicekeysmessage_p.h"
#include "newkeymessage_p.h"

using namespace QtDataSync;

#define REGISTER(x) do { \
	qRegisterMetaType<x>(#x); \
	qRegisterMetaTypeStreamOperators<x>(#x); \
} while(false)
#define REGISTER_LIST(x) do { \
	REGISTER(x); \
	REGISTER(QList<x>); \
} while(false)

const QByteArray Message::PingMessage(1, '\xFF');

void Message::registerTypes()
{
	qRegisterMetaType<QVersionNumber>();
	qRegisterMetaTypeStreamOperators<QVersionNumber>(); //whyever this is not done by Qt itself...

	qRegisterMetaType<Utf8String>();
	qRegisterMetaTypeStreamOperators<Utf8String>();
	REGISTER_LIST(QtDataSync::DevicesMessage::DeviceInfo);
	REGISTER_LIST(QtDataSync::DeviceKeysMessage::DeviceKey);
	REGISTER_LIST(QtDataSync::NewKeyMessage::KeyUpdate);
}

Message::~Message() {}

const QMetaObject *Message::metaObject() const
{
	return getMetaObject();
}

QByteArray Message::messageName() const
{
	return msgNameImpl(getMetaObject());
}

QByteArray Message::typeName() const
{
	return getMetaObject()->className();
}

QByteArray Message::typeName(const QByteArray &messageName)
{
	return "QtDataSync::" + messageName + "Message";
}

void Message::serializeTo(QDataStream &stream, bool withName) const
{
	if(withName)
		stream << messageName();
	stream << *this;
	if(stream.status() != QDataStream::Ok)
		throw DataStreamException(stream);
}

QByteArray Message::serialize() const
{
	QByteArray out;
	QDataStream stream(&out, QIODevice::WriteOnly | QIODevice::Unbuffered);
	setupStream(stream);
	serializeTo(stream);
	return out;
}

QByteArray Message::serializeSigned(const CryptoPP::PKCS8PrivateKey &key, CryptoPP::RandomNumberGenerator &rng, AsymmetricCrypto *crypto) const
{
	QByteArray out;
	QDataStream stream(&out, QIODevice::WriteOnly | QIODevice::Unbuffered); //unbuffered needed for signature
	setupStream(stream);
	serializeTo(stream);
	stream << crypto->sign(key, rng, out);
	return out;
}

void Message::setupStream(QDataStream &stream)
{
	static_assert(QDataStream::Qt_DefaultCompiledVersion == QDataStream::Qt_5_6, "update DS version");
	stream.setVersion(QDataStream::Qt_5_6);
	stream.resetStatus();
}

void Message::deserializeMessageTo(QDataStream &stream, Message &message)
{
	stream.startTransaction();
	stream >> message;
	if(message.validate()) {
		if(stream.commitTransaction())
			return;
	} else
		stream.abortTransaction();
	throw DataStreamException(stream);
}

void Message::verifySignature(QDataStream &stream, const CryptoPP::X509PublicKey &key, AsymmetricCrypto *crypto)
{
	auto device = stream.device();
	auto cPos = device->pos();

	stream.startTransaction();
	QByteArray signature;
	stream >> signature;
	if(!stream.commitTransaction())
		throw DataStreamException(stream);

	auto nPos = device->pos();
	device->reset();
	auto msgData = device->read(cPos);
	device->seek(nPos);

	crypto->verify(key, msgData, signature);
}

bool Message::validate()
{
	return true;
}

QByteArray Message::msgNameImpl(const QMetaObject *metaObject)
{
	QByteArray name(metaObject->className());
	Q_ASSERT_X(name.startsWith("QtDataSync::"), Q_FUNC_INFO, "Message is not in QtDataSync namespace");
	Q_ASSERT_X(name.endsWith("Message"), Q_FUNC_INFO, "Message does not have the Message suffix");
	name = name.mid(12); //strlen("QtDataSync::")
	name.chop(7); //strlen("Message")
	return name;
}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const Message &message)
{
	//seralize all properties in order, without type information
	auto mo = message.metaObject();
	for(auto i = 0; i < mo->propertyCount(); i++) {
		auto prop = mo->property(i);
		auto tId = prop.userType();
		auto data = prop.readOnGadget(&message);
		QMetaType::save(stream, tId, data.constData());
	}
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, Message &message)
{
	//deseralize all properties in order, without type information
	auto mo = message.metaObject();
	for(auto i = 0; i < mo->propertyCount(); i++) {
		auto prop = mo->property(i);
		auto tId = prop.userType();

		QVariant tData(tId, nullptr);
		QMetaType::load(stream, tId, tData.data());
		prop.writeOnGadget(&message, tData);
	}
	return stream;
}



DataStreamException::DataStreamException(QDataStream &stream) :
	_status(stream.status())
{
	stream.resetStatus();
}

DataStreamException::DataStreamException(QDataStream::Status status) :
	_status(status)
{}

const char *DataStreamException::what() const noexcept
{
	switch (_status) {
	case QDataStream::Ok:
		return "Unknown Error";
	case QDataStream::ReadPastEnd:
		return "Incomplete message received";
	case QDataStream::ReadCorruptData:
		return "Invalid message received";
	case QDataStream::WriteFailed:
		return "Writing message failed";
	default:
		Q_UNREACHABLE();
		break;
	}
}

void DataStreamException::raise() const
{
	throw (*this);
}

QException *DataStreamException::clone() const
{
	return new DataStreamException(_status);
}



Utf8String::Utf8String() :
	QString()
{}

Utf8String::Utf8String(const QByteArray &data) :
	QString(QString::fromUtf8(data))
{}

Utf8String::Utf8String(const QString &other) :
	QString(other)
{}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const Utf8String &message)
{
	stream << message.toUtf8();
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, Utf8String &message)
{
	stream.startTransaction();
	QByteArray m;
	stream >> m;
	message = m;
	if(message.toUtf8() == m)
		stream.commitTransaction();
	else
		stream.abortTransaction();
	return stream;
}
