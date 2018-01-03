#ifndef ACCESSMESSAGE_P_H
#define ACCESSMESSAGE_P_H

#include <QtCore/QUuid>

#include "message_p.h"
#include "registermessage_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT AccessMessage : public RegisterBaseMessage
{
	Q_GADGET

	Q_PROPERTY(QByteArray pNonce MEMBER pNonce)
	Q_PROPERTY(QUuid partnerId MEMBER partnerId)
	Q_PROPERTY(bool trusted MEMBER trusted)
	Q_PROPERTY(QByteArray signature MEMBER signature)

public:
	AccessMessage();
	AccessMessage(const QString &deviceName,
				  const QByteArray &nonce,
				  const QSharedPointer<CryptoPP::X509PublicKey> &signKey,
				  const QSharedPointer<CryptoPP::X509PublicKey> &cryptKey,
				  AsymmetricCrypto *crypto,
				  const QByteArray &pNonce,
				  const QUuid &partnerId,
				  bool trusted,
				  const QByteArray &signature);

	QByteArray pNonce;
	QUuid partnerId;
	bool trusted;
	QByteArray signature;
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const AccessMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, AccessMessage &message);

}

Q_DECLARE_METATYPE(QtDataSync::AccessMessage)

#endif // ACCESSMESSAGE_P_H
