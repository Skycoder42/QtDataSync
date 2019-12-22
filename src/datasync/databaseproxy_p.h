#ifndef QTDATASYNC_DATABASEPROXY_H
#define QTDATASYNC_DATABASEPROXY_H

#include "QtDataSync/qtdatasync_global.h"

#include <QtCore/qobject.h>

namespace QtDataSync {

class Q_DATASYNC_EXPORT DatabaseProxy : public QObject
{
	Q_OBJECT

public:
	explicit DatabaseProxy(QObject *parent = nullptr);
};

}

#endif // QTDATASYNC_DATABASEPROXY_H
