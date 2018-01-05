#include "proofmessage_p.h"
using namespace QtDataSync;

ProofMessage::ProofMessage() :
	pNonce(),
	deviceId(),
	deviceName(),
	signAlgorithm(),
	signKey(),
	cryptAlgorithm(),
	cryptKey(),
	macscheme(),
	cmac(),
	trustmac()
{}

ProofMessage::ProofMessage(const AccessMessage &access, const QUuid &deviceId) :
	pNonce(access.pNonce),
	deviceId(deviceId),
	deviceName(access.deviceName),
	signAlgorithm(access.signAlgorithm),
	signKey(access.signKey),
	cryptAlgorithm(access.cryptAlgorithm),
	cryptKey(access.cryptKey),
	macscheme(access.macscheme),
	cmac(access.cmac),
	trustmac(access.trustmac)
{}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const QtDataSync::ProofMessage &message)
{
	stream << message.pNonce
		   << message.deviceId
		   << message.deviceName
		   << message.signAlgorithm
		   << message.signKey
		   << message.cryptAlgorithm
		   << message.cryptKey
		   << message.macscheme
		   << message.cmac
		   << message.trustmac;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, QtDataSync::ProofMessage &message)
{
	stream.startTransaction();
	stream >> message.pNonce
		   >> message.deviceId
		   >> message.deviceName
		   >> message.signAlgorithm
		   >> message.signKey
		   >> message.cryptAlgorithm
		   >> message.cryptKey
		   >> message.macscheme
		   >> message.cmac
		   >> message.trustmac;
	stream.commitTransaction();
	return stream;
}



DenyMessage::DenyMessage(const QUuid &deviceId) :
	deviceId(deviceId)
{}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const DenyMessage &message)
{
	stream << message.deviceId;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, DenyMessage &message)
{
	stream.startTransaction();
	stream >> message.deviceId;
	stream.commitTransaction();
	return stream;
}



AcceptMessage::AcceptMessage(const QUuid &deviceId) :
	deviceId(deviceId),
	index(0),
	scheme(),
	secret()
{}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const AcceptMessage &message)
{
	stream << message.deviceId
		   << message.index
		   << message.scheme
		   << message.secret;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, AcceptMessage &message)
{
	stream.startTransaction();
	stream >> message.deviceId
		   >> message.index
		   >> message.scheme
		   >> message.secret;
	stream.commitTransaction();
	return stream;
}
