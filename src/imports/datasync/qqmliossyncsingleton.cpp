#include "qqmliossyncsingleton.h"
using namespace QtDataSync;

QQmlIosSyncSingleton::QQmlIosSyncSingleton(QObject *parent) :
	QObject{parent}
{}

IosSyncDelegate *QQmlIosSyncSingleton::delegate() const
{
	return IosSyncDelegate::currentDelegate();
}
