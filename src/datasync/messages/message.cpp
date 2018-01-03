#include "message_p.h"
#include "asymmetriccrypto_p.h"

#include <QtCore/QMetaProperty>

using namespace QtDataSync;

const QByteArray QtDataSync::PingMessage(1, (char)0xFF);

void QtDataSync::setupStream(QDataStream &stream)
{
	static_assert(QDataStream::Qt_DefaultCompiledVersion == QDataStream::Qt_5_6, "update DS version");
	stream.setVersion(QDataStream::Qt_5_6);
	stream.resetStatus();
}

void QtDataSync::verifySignature(QDataStream &stream, const CryptoPP::X509PublicKey &key, AsymmetricCrypto *crypto)
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

QByteArray QtDataSync::createSignature(const QByteArray &message, const CryptoPP::PKCS8PrivateKey &key, CryptoPP::RandomNumberGenerator &rng, AsymmetricCrypto *crypto)
{
	return crypto->sign(key, rng, message);
}

QByteArray QtDataSync::typeName(const QByteArray &messageName)
{
	return "QtDataSync::" + messageName + "Message";
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
	stream.commitTransaction();
	return stream;
}
