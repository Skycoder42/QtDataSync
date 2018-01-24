#ifndef QTDATASYNC_KEYCHANGEMESSAGE_P_H
#define QTDATASYNC_KEYCHANGEMESSAGE_P_H

#include "message_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT KeyChangeMessage : public Message
{
	Q_GADGET

	Q_PROPERTY(quint32 nextIndex MEMBER nextIndex)

public:
	KeyChangeMessage(quint32 nextIndex = 0);

	quint32 nextIndex;

protected:
	const QMetaObject *getMetaObject() const override;
};

}

Q_DECLARE_METATYPE(QtDataSync::KeyChangeMessage)

#endif // QTDATASYNC_KEYCHANGEMESSAGE_P_H
