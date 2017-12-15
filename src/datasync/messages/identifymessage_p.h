#ifndef IDENTIFYMESSAGE_H
#define IDENTIFYMESSAGE_H

#include <cryptopp/rng.h>

#include "message_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT IdentifyMessage
{
	Q_GADGET

	Q_PROPERTY(QByteArray nonce MEMBER nonce)

public:
	static const int NonceSize = 16;
	IdentifyMessage();

	static IdentifyMessage createRandom(CryptoPP::RandomNumberGenerator &rng);

	QByteArray nonce;
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const IdentifyMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, IdentifyMessage &message);

}

Q_DECLARE_METATYPE(QtDataSync::IdentifyMessage)

#endif // IDENTIFYMESSAGE_H
