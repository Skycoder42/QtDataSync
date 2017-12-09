#ifndef REGISTERMESSAGE_H
#define REGISTERMESSAGE_H

#include "message_p.h"

#include <cryptopp/rsa.h>

namespace QtDataSync {

class Q_DATASYNC_EXPORT RegisterMessage
{
	Q_GADGET

	Q_PROPERTY(CryptoPP::RSA::PublicKey pubKey READ getPubKey WRITE setPubKey)

public:
	RegisterMessage(const CryptoPP::RSA::PublicKey &pubKey = {});

	CryptoPP::RSA::PublicKey pubKey;

private:
	CryptoPP::RSA::PublicKey getPubKey() const;
	void setPubKey(const CryptoPP::RSA::PublicKey &value);
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const RegisterMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, RegisterMessage &message);
Q_DATASYNC_EXPORT QDebug operator<<(QDebug debug, const RegisterMessage &message);

}

Q_DECLARE_METATYPE(QtDataSync::RegisterMessage)

#endif // REGISTERMESSAGE_H
