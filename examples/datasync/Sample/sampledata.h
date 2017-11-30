#ifndef SAMPLEDATA_H
#define SAMPLEDATA_H

#include <QDebug>
#include <QObject>

class SampleData
{
	Q_GADGET

	Q_PROPERTY(int id MEMBER id USER true)
	Q_PROPERTY(QString title MEMBER title)
	Q_PROPERTY(QString description MEMBER description)

public:
	int id;
	QString title;
	QString description;
};

QDebug operator<<(QDebug debug, const SampleData &data);

Q_DECLARE_METATYPE(SampleData)

#endif // SAMPLEDATA_H
