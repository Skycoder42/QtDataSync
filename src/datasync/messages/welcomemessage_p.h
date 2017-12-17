#ifndef WELCOMEMESSAGE_P_H
#define WELCOMEMESSAGE_P_H

#include "message_p.h"
#include "syncmessage_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT WelcomeMessage : public SyncMessage
{
	Q_GADGET

public:
	WelcomeMessage(bool hasChanges = false);
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const WelcomeMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, WelcomeMessage &message);

}

Q_DECLARE_METATYPE(QtDataSync::WelcomeMessage)

#endif // WELCOMEMESSAGE_P_H
