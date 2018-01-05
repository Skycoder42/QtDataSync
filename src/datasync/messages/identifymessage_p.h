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

class Q_DATASYNC_EXPORT InitMessage
{
	Q_GADGET

	Q_PROPERTY(QVersionNumber protocolVersion MEMBER protocolVersion)
	Q_PROPERTY(QByteArray nonce MEMBER nonce)

public:
	static const QVersionNumber CurrentVersion;
	static const QVersionNumber CompatVersion;
	static const int NonceSize = 16;
	InitMessage();

	QVersionNumber protocolVersion;
	QByteArray nonce;

protected:
	InitMessage(const QByteArray &nonce);
};

class Q_DATASYNC_EXPORT IdentifyMessage : public InitMessage
{
	Q_GADGET

	Q_PROPERTY(quint32 uploadLimit MEMBER uploadLimit)

public:
	IdentifyMessage(quint32 uploadLimit = 0);

	static IdentifyMessage createRandom(quint32 uploadLimit, CryptoPP::RandomNumberGenerator &rng);

	quint32 uploadLimit;
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const InitMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, InitMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const IdentifyMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, IdentifyMessage &message);

}

Q_DECLARE_METATYPE(QtDataSync::InitMessage)
Q_DECLARE_METATYPE(QtDataSync::IdentifyMessage)

#endif // IDENTIFYMESSAGE_H
