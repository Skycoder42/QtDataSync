#ifndef QTDATASYNC_SYNCMESSAGE_P_H
#define QTDATASYNC_SYNCMESSAGE_P_H

#include "message_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT SyncMessage : public Message
{
	Q_GADGET

public:
	SyncMessage();

protected:
	const QMetaObject *getMetaObject() const override;
};

}

Q_DECLARE_METATYPE(QtDataSync::SyncMessage)

#endif // QTDATASYNC_SYNCMESSAGE_P_H
