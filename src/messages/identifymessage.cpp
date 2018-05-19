#include "identifymessage_p.h"

using namespace QtDataSync;
#if CRYPTOPP_VERSION >= 600
using byte = CryptoPP::byte;
#endif

const QVersionNumber InitMessage::CurrentVersion(1); //NOTE update accordingly
const QVersionNumber InitMessage::CompatVersion(1);

InitMessage::InitMessage() :
	InitMessage(QByteArray{})
{}

InitMessage::InitMessage(QByteArray nonce) :
	protocolVersion(CurrentVersion),
	nonce(std::move(nonce))
{}

const QMetaObject *InitMessage::getMetaObject() const
{
	return &staticMetaObject;
}

bool InitMessage::validate()
{
	if(protocolVersion < InitMessage::CompatVersion)
		throw IncompatibleVersionException(protocolVersion);
	return nonce.size() >= InitMessage::NonceSize;
}



IdentifyMessage::IdentifyMessage(quint32 uploadLimit) :
	InitMessage(),
	uploadLimit(uploadLimit)
{}

IdentifyMessage IdentifyMessage::createRandom(quint32 uploadLimit, CryptoPP::RandomNumberGenerator &rng)
{
	IdentifyMessage msg(uploadLimit);
	msg.nonce.resize(NonceSize);
	rng.GenerateBlock(reinterpret_cast<byte*>(msg.nonce.data()),
					  static_cast<size_t>(msg.nonce.size()));
	return msg;
}

const QMetaObject *IdentifyMessage::getMetaObject() const
{
	return &staticMetaObject;
}



IncompatibleVersionException::IncompatibleVersionException(QVersionNumber invalidVersion) :
	_version(std::move(invalidVersion)),
	_msg(QStringLiteral("Incompatible protocol versions. Must be at least %1, but remote proposed with %2")
		 .arg(InitMessage::CompatVersion.toString(), _version.toString())
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
