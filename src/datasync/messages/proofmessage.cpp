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

const QMetaObject *ProofMessage::getMetaObject() const
{
	return &staticMetaObject;
}



DenyMessage::DenyMessage(const QUuid &deviceId) :
	deviceId(deviceId)
{}

const QMetaObject *DenyMessage::getMetaObject() const
{
	return &staticMetaObject;
}



AcceptMessage::AcceptMessage(const QUuid &deviceId) :
	deviceId(deviceId),
	index(0),
	scheme(),
	secret()
{}

const QMetaObject *AcceptMessage::getMetaObject() const
{
	return &staticMetaObject;
}



AcceptAckMessage::AcceptAckMessage(const QUuid &deviceId) :
	deviceId(deviceId)
{}

const QMetaObject *AcceptAckMessage::getMetaObject() const
{
	return &staticMetaObject;
}
