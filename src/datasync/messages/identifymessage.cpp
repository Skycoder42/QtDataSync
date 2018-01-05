#include "identifymessage_p.h"

using namespace QtDataSync;

const QVersionNumber InitMessage::CurrentVersion(1, 0, 0); //NOTE update accordingly
const QVersionNumber InitMessage::CompatVersion(1, 0, 0);

InitMessage::InitMessage() :
	InitMessage(QByteArray())
{}

InitMessage::InitMessage(const QByteArray &nonce) :
	protocolVersion(CurrentVersion),
	nonce(nonce)
{}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const InitMessage &message)
{
	stream << message.protocolVersion
		   << message.nonce;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, InitMessage &message)
{
	stream.startTransaction();
	stream >> message.protocolVersion
		   >> message.nonce;
	if(message.protocolVersion < InitMessage::CompatVersion)
		throw IncompatibleVersionException(message.protocolVersion);
	if(message.nonce.size() < InitMessage::NonceSize)
		stream.abortTransaction();
	else
		stream.commitTransaction();
	return stream;
}



IdentifyMessage::IdentifyMessage(quint32 uploadLimit) :
	InitMessage(),
	uploadLimit(uploadLimit)
{}

IdentifyMessage IdentifyMessage::createRandom(quint32 uploadLimit, CryptoPP::RandomNumberGenerator &rng)
{
	IdentifyMessage msg(uploadLimit);
	msg.nonce.resize(NonceSize);
	rng.GenerateBlock(reinterpret_cast<byte*>(msg.nonce.data()), msg.nonce.size());
	return msg;
}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const IdentifyMessage &message)
{
	stream << static_cast<InitMessage>(message)
		   << message.uploadLimit;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, IdentifyMessage &message)
{
	stream.startTransaction();
	stream >> static_cast<InitMessage&>(message)
		   >> message.uploadLimit;
	stream.commitTransaction();
	return stream;
}




IncompatibleVersionException::IncompatibleVersionException(const QVersionNumber &invalidVersion) :
	_version(invalidVersion),
	_msg(QStringLiteral("Incompatible protocol versions. Must be at least %1, but remote proposed with %2")
		 .arg(InitMessage::CompatVersion.toString())
		 .arg(_version.toString())
		 .toUtf8())
{}

QVersionNumber IncompatibleVersionException::invalidVersion() const
{
	return _version;
}

const char *IncompatibleVersionException::what() const noexcept
{
	return _msg.constData();
}

void IncompatibleVersionException::raise() const
{
	throw (*this);
}

QException *IncompatibleVersionException::clone() const
{
	return new IncompatibleVersionException(_version);
}
