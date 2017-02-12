#ifndef DATABASECONTROLLER_H
#define DATABASECONTROLLER_H

#include <QObject>

class DatabaseController : public QObject
{
	Q_OBJECT

public:
	explicit DatabaseController(QObject *parent = nullptr);
};

#endif // DATABASECONTROLLER_H
