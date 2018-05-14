#ifndef QTDATASYNC_IDENTIFYMESSAGE_H
#define QTDATASYNC_IDENTIFYMESSAGE_H

#include <QtCore/QVersionNumber>
#include <QtCore/QException>

#include <cryptopp/rng.h>

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

class Q_DATASYNC_EXPORT InitMessage : public Message
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

	const QMetaObject *getMetaObject() const override;
	bool validate() override;
};

class Q_DATASYNC_EXPORT IdentifyMessage : public InitMessage
{
	Q_GADGET

	Q_PROPERTY(quint32 uploadLimit MEMBER uploadLimit)

public:
	IdentifyMessage(quint32 uploadLimit = 0);

	static IdentifyMessage createRandom(quint32 uploadLimit, CryptoPP::RandomNumberGenerator &rng);

	quint32 uploadLimit;

protected:
	const QMetaObject *getMetaObject() const override;
};

}

Q_DECLARE_METATYPE(QtDataSync::InitMessage)
Q_DECLARE_METATYPE(QtDataSync::IdentifyMessage)

#endif // QTDATASYNC_IDENTIFYMESSAGE_H
