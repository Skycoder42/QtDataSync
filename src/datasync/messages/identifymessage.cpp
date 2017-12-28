#include "identifymessage_p.h"

using namespace QtDataSync;

const QVersionNumber IdentifyMessage::CurrentVersion(1, 0, 0); //NOTE update accordingly
const QVersionNumber IdentifyMessage::CompatVersion(1, 0, 0);

IdentifyMessage::IdentifyMessage() :
	IdentifyMessage(QByteArray())
{}

IdentifyMessage::IdentifyMessage(const QByteArray &nonce) :
	protocolVersion(CurrentVersion),
	nonce(nonce)
{}

IdentifyMessage IdentifyMessage::createRandom(CryptoPP::RandomNumberGenerator &rng)
{
	IdentifyMessage msg;
	msg.nonce.resize(NonceSize);
	rng.GenerateBlock((byte*)msg.nonce.data(), msg.nonce.size());
	return msg;
}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const IdentifyMessage &message)
{
	stream << message.protocolVersion
		   << message.nonce;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, IdentifyMessage &message)
{
	stream.startTransaction();
	stream >> message.protocolVersion
		   >> message.nonce;
	if(message.protocolVersion < IdentifyMessage::CompatVersion)
		throw IncompatibleVersionException(message.protocolVersion);
	if(message.nonce.size() < IdentifyMessage::NonceSize)
		stream.abortTransaction();
	else
		stream.commitTransaction();
	return stream;
}



IncompatibleVersionException::IncompatibleVersionException(const QVersionNumber &invalidVersion) :
	_version(invalidVersion),
	_msg(QStringLiteral("Incompatible protocol versions. Must be at least %1, but remote proposed with %2")
		 .arg(IdentifyMessage::CompatVersion.toString())
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
