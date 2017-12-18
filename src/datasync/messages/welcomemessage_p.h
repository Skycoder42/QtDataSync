#ifndef WELCOMEMESSAGE_P_H
#define WELCOMEMESSAGE_P_H

#include "message_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT WelcomeMessage
{
	Q_GADGET

	Q_PROPERTY(bool hasChanges MEMBER hasChanges)

public:
	WelcomeMessage(bool hasChanges = false);

	bool hasChanges;
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const WelcomeMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, WelcomeMessage &message);

}

Q_DECLARE_METATYPE(QtDataSync::WelcomeMessage)

#endif // WELCOMEMESSAGE_P_H
