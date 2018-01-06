#ifndef REGISTERMESSAGE_H
#define REGISTERMESSAGE_H

#include <functional>

#include <cryptopp/rsa.h>

#include "message_p.h"
#include "asymmetriccrypto_p.h"
#include "identifymessage_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT RegisterBaseMessage : public InitMessage
{
	Q_GADGET

	Q_PROPERTY(QByteArray signAlgorithm MEMBER signAlgorithm)
	Q_PROPERTY(QByteArray signKey MEMBER signKey)
	Q_PROPERTY(QByteArray cryptAlgorithm MEMBER cryptAlgorithm)
	Q_PROPERTY(QByteArray cryptKey MEMBER cryptKey)
	Q_PROPERTY(Utf8String deviceName MEMBER deviceName)

public:
	RegisterBaseMessage();
	RegisterBaseMessage(const QString &deviceName,
						const QByteArray &nonce,
						const QSharedPointer<CryptoPP::X509PublicKey> &signKey,
						const QSharedPointer<CryptoPP::X509PublicKey> &cryptKey,
						AsymmetricCrypto *crypto);

	QByteArray signAlgorithm;
	QByteArray signKey;
	QByteArray cryptAlgorithm;
	QByteArray cryptKey;
	Utf8String deviceName;

	AsymmetricCryptoInfo *createCryptoInfo(CryptoPP::RandomNumberGenerator &rng, QObject *parent = nullptr) const;
};

class Q_DATASYNC_EXPORT RegisterMessage : public RegisterBaseMessage
{
	Q_GADGET

	Q_PROPERTY(QByteArray cmac MEMBER cmac)

public:
	RegisterMessage();
	RegisterMessage(const QString &deviceName,
					const QByteArray &nonce,
					const QSharedPointer<CryptoPP::X509PublicKey> &signKey,
					const QSharedPointer<CryptoPP::X509PublicKey> &cryptKey,
					AsymmetricCrypto *crypto,
					const QByteArray &cmac);

	QByteArray cmac;
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const RegisterBaseMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, RegisterBaseMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const RegisterMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, RegisterMessage &message);

}

Q_DECLARE_METATYPE(QtDataSync::RegisterBaseMessage)
Q_DECLARE_METATYPE(QtDataSync::RegisterMessage)

#endif // REGISTERMESSAGE_H
