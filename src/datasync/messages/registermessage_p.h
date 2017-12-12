#ifndef REGISTERMESSAGE_H
#define REGISTERMESSAGE_H

#include "message_p.h"
#include "asymmetriccrypto_p.h"

#include <functional>

#include <QScopedPointer>

#include <cryptopp/rsa.h>

namespace QtDataSync {

class Q_DATASYNC_EXPORT RegisterMessage
{
	Q_GADGET

	Q_PROPERTY(QByteArray keyAlgorithm MEMBER keyAlgorithm)
	Q_PROPERTY(QByteArray pubKey MEMBER pubKey)
	Q_PROPERTY(QString deviceName MEMBER deviceName)

public:
	RegisterMessage();
	RegisterMessage(const QString &deviceName,
					const CryptoPP::X509PublicKey &pubKey,
					AsymmetricCrypto *crypto);

	QByteArray keyAlgorithm;
	QByteArray pubKey;
	QString deviceName;

	AsymmetricCrypto *createCrypto(QObject *parent = nullptr);
	QSharedPointer<CryptoPP::X509PublicKey> getKey(AsymmetricCrypto *crypto);
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const RegisterMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, RegisterMessage &message);
Q_DATASYNC_EXPORT QDebug operator<<(QDebug debug, const RegisterMessage &message);

}

Q_DECLARE_METATYPE(QtDataSync::RegisterMessage)

#endif // REGISTERMESSAGE_H
