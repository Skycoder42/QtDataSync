#ifndef QQMLDATASTORE_H
#define QQMLDATASTORE_H

#include <QtCore/QObject>

#include <QtQml/QQmlParserStatus>

#include <QtDataSync/datastore.h>

namespace QtDataSync {

class QQmlDataStore : public DataStore, public QQmlParserStatus
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

	Q_INVOKABLE qint64 count(const QString &typeName) const;
	Q_INVOKABLE QStringList keys(const QString &typeName) const;
	Q_INVOKABLE QVariantList loadAll(const QString &typeName) const;
	Q_INVOKABLE QVariant load(const QString &typeName, const QString &key) const;
	Q_INVOKABLE void save(const QString &typeName, QVariant value);
	Q_INVOKABLE bool remove(const QString &typeName, const QString &key);
	Q_INVOKABLE QVariantList search(const QString &typeName, const QString &query, DataStore::SearchMode mode = DataStore::RegexpMode) const;
	Q_INVOKABLE void clear(const QString &typeName);

	Q_INVOKABLE QString typeName(int typeId) const;

public Q_SLOTS:
	void setSetupName(QString setupName);

Q_SIGNALS:
	void setupNameChanged(QString setupName);
	void validChanged(bool valid);

private:
	QString _setupName;
	bool _valid;
};

}

#endif // QQMLDATASTORE_H
