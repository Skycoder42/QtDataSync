#ifndef GRANTMESSAGE_P_H
#define GRANTMESSAGE_P_H

#include "message_p.h"
#include "accountmessage_p.h"
#include "proofmessage_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT GrantMessage : public AccountMessage
{
	Q_GADGET

	Q_PROPERTY(quint32 index MEMBER index)
	Q_PROPERTY(QByteArray scheme MEMBER scheme)
	Q_PROPERTY(QByteArray secret MEMBER secret)

public:
	GrantMessage();
	GrantMessage(const QUuid &deviceId, const AcceptMessage &message);

	quint32 index;
	QByteArray scheme;
	QByteArray secret;

protected:
	const QMetaObject *getMetaObject() const override;
};

}

Q_DECLARE_METATYPE(QtDataSync::GrantMessage)

#endif // GRANTMESSAGE_P_H
