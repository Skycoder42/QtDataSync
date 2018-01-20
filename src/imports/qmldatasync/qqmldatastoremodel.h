#ifndef QQMLDATASTOREMODEL_H
#define QQMLDATASTOREMODEL_H

#include <QtCore/QObject>

#include <QtQml/QQmlParserStatus>

#include <QtDataSync/datastoremodel.h>

#include "qqmldatastore.h"

namespace QtDataSync {

class QQmlDataStoreModel : public DataStoreModel, public QQmlParserStatus
{
	Q_OBJECT
	Q_DISABLE_COPY(QQmlDataStoreModel)
	Q_INTERFACES(QQmlParserStatus)

	Q_PROPERTY(QString setupName READ setupName WRITE setSetupName NOTIFY setupNameChanged)
	Q_PROPERTY(QQmlDataStore* dataStore READ dataStore WRITE setDataStore NOTIFY dataStoreChanged)
	Q_PROPERTY(bool valid READ valid NOTIFY validChanged)
	Q_PROPERTY(QString typeName READ typeName WRITE setTypeName NOTIFY typeNameChanged)

public:
	explicit QQmlDataStoreModel(QObject *parent = nullptr);

	void classBegin() override;
	void componentComplete() override;

	QString setupName() const;
	QQmlDataStore* dataStore() const;
	bool valid() const;
	QString typeName() const;

public Q_SLOTS:
	void setSetupName(QString setupName);
	void setDataStore(QQmlDataStore* dataStore);
	void setTypeName(QString typeName);

Q_SIGNALS:
	void setupNameChanged(QString setupName);
	void dataStoreChanged(QQmlDataStore* dataStore);
	void validChanged(bool valid);
	void typeNameChanged(QString typeName);

private:
	QString _setupName;
	QQmlDataStore* _dataStore;
};

}

#endif // QQMLDATASTOREMODEL_H
