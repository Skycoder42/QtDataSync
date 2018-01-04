#ifndef PROOFMESSAGE_P_H
#define PROOFMESSAGE_P_H

#include "message_p.h"
#include "accessmessage_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT ProofMessage
{
	Q_GADGET

	Q_PROPERTY(QByteArray pNonce MEMBER pNonce)
	Q_PROPERTY(QUuid deviceId MEMBER deviceId)
	Q_PROPERTY(QByteArray signAlgorithm MEMBER signAlgorithm)
	Q_PROPERTY(QByteArray signKey MEMBER signKey)
	Q_PROPERTY(QByteArray cryptAlgorithm MEMBER cryptAlgorithm)
	Q_PROPERTY(QByteArray cryptKey MEMBER cryptKey)
	Q_PROPERTY(QByteArray macscheme MEMBER macscheme)
	Q_PROPERTY(QByteArray cmac MEMBER cmac)
	Q_PROPERTY(QByteArray trustmac MEMBER trustmac)

public:
	ProofMessage();
	ProofMessage(const AccessMessage &access, const QUuid &deviceId);

	QByteArray pNonce;
	QUuid deviceId;
	QByteArray signAlgorithm;
	QByteArray signKey;
	QByteArray cryptAlgorithm;
	QByteArray cryptKey;
	QByteArray macscheme;
	QByteArray cmac;
	QByteArray trustmac;
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const ProofMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, ProofMessage &message);

}

Q_DECLARE_METATYPE(QtDataSync::ProofMessage)

#endif // PROOFMESSAGE_P_H
