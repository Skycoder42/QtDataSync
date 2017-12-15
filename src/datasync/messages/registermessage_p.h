#ifndef REGISTERMESSAGE_H
#define REGISTERMESSAGE_H

#include <functional>

#include <cryptopp/rsa.h>

#include "message_p.h"
#include "asymmetriccrypto_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT RegisterMessage
{
	Q_GADGET

	Q_PROPERTY(QByteArray signAlgorithm MEMBER signAlgorithm)
	Q_PROPERTY(QByteArray signKey MEMBER signKey)
	Q_PROPERTY(QByteArray cryptAlgorithm MEMBER cryptAlgorithm)
	Q_PROPERTY(QByteArray cryptKey MEMBER cryptKey)
	Q_PROPERTY(QString deviceName MEMBER deviceName)
	Q_PROPERTY(QByteArray nonce MEMBER nonce)

public:
	RegisterMessage();
	RegisterMessage(const QString &deviceName,
					const QByteArray &nonce,
					const QSharedPointer<CryptoPP::X509PublicKey> &signKey,
					const QSharedPointer<CryptoPP::X509PublicKey> &cryptKey,
					AsymmetricCrypto *crypto);

	QByteArray signAlgorithm;
	QByteArray signKey;
	QByteArray cryptAlgorithm;
	QByteArray cryptKey;
	QString deviceName;
	QByteArray nonce;

	AsymmetricCryptoInfo *createCryptoInfo(CryptoPP::RandomNumberGenerator &rng, QObject *parent = nullptr) const;
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const RegisterMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, RegisterMessage &message);

}

Q_DECLARE_METATYPE(QtDataSync::RegisterMessage)

#endif // REGISTERMESSAGE_H
