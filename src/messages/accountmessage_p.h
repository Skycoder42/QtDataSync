#ifndef QTDATASYNC_ACCOUNTMESSAGE_P_H
#define QTDATASYNC_ACCOUNTMESSAGE_P_H

#include <QtCore/QUuid>

#include <cryptopp/rng.h>

#include "message_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT AccountMessage : public Message
{
	Q_GADGET

	Q_PROPERTY(QUuid deviceId MEMBER deviceId)

public:
	AccountMessage(const QUuid &deviceId = {});

	QUuid deviceId;

protected:
	const QMetaObject *getMetaObject() const override;
};

}

Q_DECLARE_METATYPE(QtDataSync::AccountMessage)

#endif // QTDATASYNC_ACCOUNTMESSAGE_P_H
