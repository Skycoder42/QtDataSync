#ifndef SYNCMESSAGE_P_H
#define SYNCMESSAGE_P_H

#include "message_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT SyncMessage
{
	Q_GADGET

	Q_PROPERTY(Action action MEMBER action)

public:
	enum Action {
		InvalidAction = 0,
		TriggerAction = 1,
		InitAction = 2,
		DoneAction = 3
	};
	Q_ENUM(Action)

	SyncMessage(Action action = InvalidAction);

	Action action;
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const SyncMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, SyncMessage &message);

}

Q_DECLARE_METATYPE(QtDataSync::SyncMessage)

#endif // SYNCMESSAGE_P_H
