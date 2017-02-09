#ifndef CHANGECONTROLLER_H
#define CHANGECONTROLLER_H

#include <QObject>

namespace QtDataSync {

class ChangeController : public QObject
{
	Q_OBJECT

public:
	explicit ChangeController(QObject *parent = nullptr);
};

}

#endif // CHANGECONTROLLER_H
