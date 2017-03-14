#ifndef SAMPLEDATA_H
#define SAMPLEDATA_H

#include <QObject>

class SampleData : public QObject
{
	Q_OBJECT

	Q_PROPERTY(int id MEMBER id USER true)
	Q_PROPERTY(QString title MEMBER title)
	Q_PROPERTY(QString description MEMBER description)

public:
	Q_INVOKABLE SampleData(QObject *parent = nullptr);

	int id;
	QString title;
	QString description;
};

#endif // SAMPLEDATA_H
