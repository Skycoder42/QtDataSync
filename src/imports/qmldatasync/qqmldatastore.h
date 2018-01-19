#ifndef QQMLDATASTORE_H
#define QQMLDATASTORE_H

#include <QtCore/QObject>

#include <QtQml/QQmlParserStatus>

#include <QtDataSync/datastore.h>

namespace QtDataSync {

class QQmlDataStore : public QObject, public QQmlParserStatus
{
	Q_OBJECT
	Q_DISABLE_COPY(QQmlDataStore)
	Q_INTERFACES(QQmlParserStatus)

	Q_PROPERTY(QString setupName READ setupName WRITE setSetupName NOTIFY setupNameChanged)

public:
	explicit QQmlDataStore(QObject *parent = nullptr);

	void classBegin() override;
	void componentComplete() override;

	QString setupName() const;

public slots:
	void setSetupName(QString setupName);

signals:
	void setupNameChanged(QString setupName);

private:
	DataStore *_store;
	QString _setupName;
};

}

#endif // QQMLDATASTORE_H
