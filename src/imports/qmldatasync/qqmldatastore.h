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
	Q_PROPERTY(bool valid READ valid NOTIFY validChanged)

public:
	explicit QQmlDataStore(QObject *parent = nullptr);

	void classBegin() override;
	void componentComplete() override;

	QString setupName() const;
	bool valid() const;

public Q_SLOTS:
	void setSetupName(QString setupName);

Q_SIGNALS:
	void setupNameChanged(QString setupName);
	void validChanged(bool valid);

private:
	DataStore *_store;
	QString _setupName;
};

}

#endif // QQMLDATASTORE_H
