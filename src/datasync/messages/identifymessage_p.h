#ifndef IDENTIFYMESSAGE_H
#define IDENTIFYMESSAGE_H

#include "message_p.h"

#include <cryptopp/rng.h>

namespace QtDataSync {

class Q_DATASYNC_EXPORT IdentifyMessage
{
	Q_GADGET

	Q_PROPERTY(quint64 nonce MEMBER nonce)

public:
	IdentifyMessage();

	static IdentifyMessage createRandom(CryptoPP::RandomNumberGenerator &rng);

	quint64 nonce;
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const IdentifyMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, IdentifyMessage &message);

}

Q_DECLARE_METATYPE(QtDataSync::IdentifyMessage)

#endif // IDENTIFYMESSAGE_H
