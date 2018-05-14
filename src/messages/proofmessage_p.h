#ifndef QTDATASYNC_PROOFMESSAGE_P_H
#define QTDATASYNC_PROOFMESSAGE_P_H

#include "message_p.h"
#include "accessmessage_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT ProofMessage : public Message
{
	Q_GADGET

	Q_PROPERTY(QByteArray pNonce MEMBER pNonce)
	Q_PROPERTY(QUuid deviceId MEMBER deviceId)
	Q_PROPERTY(QString deviceName MEMBER deviceName)
	Q_PROPERTY(QByteArray signAlgorithm MEMBER signAlgorithm)
	Q_PROPERTY(QByteArray signKey MEMBER signKey)
	Q_PROPERTY(QByteArray cryptAlgorithm MEMBER cryptAlgorithm)
	Q_PROPERTY(QByteArray cryptKey MEMBER cryptKey)
	Q_PROPERTY(QByteArray macscheme MEMBER macscheme)
	Q_PROPERTY(QByteArray cmac MEMBER cmac)
	Q_PROPERTY(QByteArray trustmac MEMBER trustmac)

public:
	ProofMessage();
	ProofMessage(const AccessMessage &access, const QUuid &deviceId);

	QByteArray pNonce;
	QUuid deviceId;
	QString deviceName;
	QByteArray signAlgorithm;
	QByteArray signKey;
	QByteArray cryptAlgorithm;
	QByteArray cryptKey;
	QByteArray macscheme;
	QByteArray cmac;
	QByteArray trustmac;

protected:
	const QMetaObject *getMetaObject() const override;
};

class Q_DATASYNC_EXPORT DenyMessage : public Message
{
	Q_GADGET

	Q_PROPERTY(QUuid deviceId MEMBER deviceId)

public:
	DenyMessage(const QUuid &deviceId = {});

	QUuid deviceId;

protected:
	const QMetaObject *getMetaObject() const override;
};

class Q_DATASYNC_EXPORT AcceptMessage : public Message
{
	Q_GADGET

	Q_PROPERTY(QUuid deviceId MEMBER deviceId)
	Q_PROPERTY(quint32 index MEMBER index)
	Q_PROPERTY(QByteArray scheme MEMBER scheme)
	Q_PROPERTY(QByteArray secret MEMBER secret)

public:
	AcceptMessage(const QUuid &deviceId = {});

	QUuid deviceId;
	quint32 index;
	QByteArray scheme;
	QByteArray secret;

protected:
	const QMetaObject *getMetaObject() const override;
};

class Q_DATASYNC_EXPORT AcceptAckMessage : public Message
{
	Q_GADGET

	Q_PROPERTY(QUuid deviceId MEMBER deviceId)

public:
	AcceptAckMessage(const QUuid &deviceId = {});

	QUuid deviceId;

protected:
	const QMetaObject *getMetaObject() const override;
};

}

Q_DECLARE_METATYPE(QtDataSync::ProofMessage)
Q_DECLARE_METATYPE(QtDataSync::DenyMessage)
Q_DECLARE_METATYPE(QtDataSync::AcceptMessage)
Q_DECLARE_METATYPE(QtDataSync::AcceptAckMessage)

#endif // QTDATASYNC_PROOFMESSAGE_P_H
