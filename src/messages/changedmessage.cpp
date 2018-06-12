#include "changedmessage_p.h"
using namespace QtDataSync;

const QMetaObject *ChangedMessage::getMetaObject() const
{
	return &staticMetaObject;
}



ChangedInfoMessage::ChangedInfoMessage(quint32 changeEstimate) :
	ChangedMessage{},
	changeEstimate{changeEstimate}
{}

const QMetaObject *ChangedInfoMessage::getMetaObject() const
{
	return &staticMetaObject;
}



const QMetaObject *LastChangedMessage::getMetaObject() const
{
	return &staticMetaObject;
}



ChangedAckMessage::ChangedAckMessage(quint64 dataIndex) :
	dataIndex{dataIndex}
{}

const QMetaObject *ChangedAckMessage::getMetaObject() const
{
	return &staticMetaObject;
}
