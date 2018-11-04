#ifndef QQMLIOSSYNCSINGLETON_H
#define QQMLIOSSYNCSINGLETON_H

#include <QtCore/QObject>

#include <QtDataSyncIos/IosSyncDelegate>

namespace QtDataSync {

class QQmlIosSyncSingleton : public QObject
{
	Q_OBJECT

public:
	explicit QQmlIosSyncSingleton(QObject *parent = nullptr);

	Q_INVOKABLE QtDataSync::IosSyncDelegate *delegate() const;
};

}

#endif // QQMLIOSSYNCSINGLETON_H
