#ifndef ACCESSMESSAGE_P_H
#define ACCESSMESSAGE_P_H

#include "message_p.h"
#include "registermessage_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT AccessMessage : public RegisterBaseMessage
{
	Q_GADGET

public:
	AccessMessage();
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const AccessMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, AccessMessage &message);

}

Q_DECLARE_METATYPE(QtDataSync::AccessMessage)

#endif // ACCESSMESSAGE_P_H
