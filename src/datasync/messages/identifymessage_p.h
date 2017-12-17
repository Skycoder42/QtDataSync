#ifndef IDENTIFYMESSAGE_H
#define IDENTIFYMESSAGE_H

#include <cryptopp/rng.h>

#include <QtCore/QVersionNumber>
#include <QtCore/QException>

#include "message_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT IncompatibleVersionException : public QException
{
public:
	IncompatibleVersionException(const QVersionNumber &invalidVersion);

	QVersionNumber invalidVersion() const;

	const char *what() const noexcept override;
	void raise() const override;
	QException *clone() const override;

private:
	const QVersionNumber _version;
	const QByteArray _msg;
};

class Q_DATASYNC_EXPORT IdentifyMessage
{
	Q_GADGET

	Q_PROPERTY(QVersionNumber protocolVersion MEMBER protocolVersion)
	Q_PROPERTY(QByteArray nonce MEMBER nonce)

public:
	static const QVersionNumber CurrentVersion;
	static const QVersionNumber CompatVersion;
	static const int NonceSize = 16;
	IdentifyMessage();

	static IdentifyMessage createRandom(CryptoPP::RandomNumberGenerator &rng);

	QVersionNumber protocolVersion;
	QByteArray nonce;

protected:
	IdentifyMessage(const QByteArray &nonce);
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const IdentifyMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, IdentifyMessage &message);

}

Q_DECLARE_METATYPE(QtDataSync::IdentifyMessage)

#endif // IDENTIFYMESSAGE_H
