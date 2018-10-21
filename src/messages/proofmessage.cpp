#include "proofmessage_p.h"
using namespace QtDataSync;

ProofMessage::ProofMessage() = default;

ProofMessage::ProofMessage(const AccessMessage &access, QUuid deviceId) :
	pNonce{access.pNonce},
	deviceId{deviceId},
	deviceName{access.deviceName},
	signAlgorithm{access.signAlgorithm},
	signKey{access.signKey},
	cryptAlgorithm{access.cryptAlgorithm},
	cryptKey{access.cryptKey},
	macscheme{access.macscheme},
	cmac{access.cmac},
	trustmac{access.trustmac}
{}

const QMetaObject *ProofMessage::getMetaObject() const
{
	return &staticMetaObject;
}



DenyMessage::DenyMessage(QUuid deviceId) :
	deviceId{deviceId}
{}

const QMetaObject *DenyMessage::getMetaObject() const
{
	return &staticMetaObject;
}



AcceptMessage::AcceptMessage(QUuid deviceId) :
	deviceId{deviceId}
{}

const QMetaObject *AcceptMessage::getMetaObject() const
{
	return &staticMetaObject;
}



AcceptAckMessage::AcceptAckMessage(QUuid deviceId) :
	deviceId{deviceId}
{}

const QMetaObject *AcceptAckMessage::getMetaObject() const
{
	return &staticMetaObject;
}
