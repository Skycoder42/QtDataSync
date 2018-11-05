#ifndef QTDATASYNC_DATASTOREMODEL_H
#define QTDATASYNC_DATASTOREMODEL_H

#include <QtCore/qabstractitemmodel.h>

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/datastore.h"

namespace QtDataSync {

class DataStoreModelPrivate;
//! A passive item model for a datasync data store
class Q_DATASYNC_EXPORT DataStoreModel : public QAbstractTableModel
{
	Q_OBJECT
	friend class DataStoreModelPrivate;

	//! Holds the type the model loads data for
	Q_PROPERTY(int typeId READ typeId WRITE setTypeId NOTIFY typeIdChanged)
	//! Specifies whether the model items can be edited
	Q_PROPERTY(bool editable READ isEditable WRITE setEditable NOTIFY editableChanged)

public:
	//! Constructs a model for the default setup
	explicit DataStoreModel(QObject *parent = nullptr);
	//! Constructs a model for the given setup
	explicit DataStoreModel(const QString &setupName, QObject *parent = nullptr);
	//! Constructs a model on the given store
	explicit DataStoreModel(DataStore *store, QObject *parent = nullptr);
	~DataStoreModel() override;

	//! Returns the data store the model works on
	DataStore *store() const;
	QString setupName() const;

	//! @readAcFn{DataStoreModel::typeId}
	int typeId() const;
	//! @writeAcFn{DataStoreModel::typeId}
	template <typename T>
	inline void setTypeId(bool resetColumns = true);
	//! @readAcFn{DataStoreModel::editable}
	bool isEditable() const;

	//! @inherit{QAbstractTableModel::headerData}
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	//! @inherit{QAbstractTableModel::rowCount}
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	//! @inherit{QAbstractTableModel::columnCount}
	int columnCount(const QModelIndex &parent) const override;
	//! @inherit{QAbstractTableModel::canFetchMore}
	bool canFetchMore(const QModelIndex &parent) const override;
	//! @inherit{QAbstractTableModel::fetchMore}
	void fetchMore(const QModelIndex &parent) override;

	//! @inherit{QAbstractTableModel::index}
	QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const override;
	//! Returns the index of the item with the given id
	Q_INVOKABLE QModelIndex idIndex(const QString &id) const;
	//! @copybrief DataStoreModel::idIndex(const QString &) const
	template <typename T>
	inline QModelIndex idIndex(const T &id) const;

	//! Returns the key of the item at the given index
	Q_INVOKABLE QString key(const QModelIndex &index) const;
	//! Returns the key of the item at the given index
	template <typename T>
	inline T key(const QModelIndex &index) const;

	//! @inherit{QAbstractTableModel::data}
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	//! @inherit{QAbstractTableModel::setData}
	bool setData(const QModelIndex &index, const QVariant &value, int role) override;

	//! Returns the object at the given index
	Q_INVOKABLE QVariant object(const QModelIndex &index) const;
	/*! @copybrief DataStoreModel::object(const QModelIndex &) const
	 * @tparam T The type of object to return. Must match DataStoreModel::typeId
	 * @copydetails DataStoreModel::object(const QModelIndex &) const
	 */
	template <typename T>
	inline T object(const QModelIndex &index) const;
	//! Loads the object at the given index from the store via DataStore::load
	Q_INVOKABLE QVariant loadObject(const QModelIndex &index) const;
	/*! @copybrief DataStoreModel::loadObject(const QModelIndex &) const
	 * @tparam T The type of object to return. Must match DataStoreModel::typeId
	 * @copydetails DataStoreModel::loadObject(const QModelIndex &) const
	 */
	template <typename T>
	T loadObject(const QModelIndex &index) const;

	//! @inherit{QAbstractTableModel::flags}
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	//! @inherit{QAbstractTableModel::roleNames}
	QHash<int, QByteArray> roleNames() const override;

	//! Add a new column with the given title
	int addColumn(const QString &text);
	//! Add a new column and set the given property as its Qt::DisplayRole
	int addColumn(const QString &text, const char *propertyName);
	//! Adds the given property as a new role to the given column
	void addRole(int column, int role, const char *propertyName);
	//! Removes all customly added columns and roles
	void clearColumns();

public Q_SLOTS:
	//! @writeAcFn{DataStoreModel::typeId}
	void setTypeId(int typeId); //MAJOR merge methods
	//! @writeAcFn{DataStoreModel::typeId}
	void setTypeId(int typeId, bool resetColumns);
	//! @writeAcFn{DataStoreModel::editable}
	void setEditable(bool editable);

	//! Reloads all data in the model
	void reload();

Q_SIGNALS:
	//! Emitted when the underlying DataStore throws an exception
	void storeError(const QException &exception, QPrivateSignal);
	//! @notifyAcFn{DataStoreModel::typeId}
	void typeIdChanged(int typeId, QPrivateSignal);
	//! @notifyAcFn{DataStoreModel::editable}
	void editableChanged(bool editable, QPrivateSignal);

protected:
	//! @private
	explicit DataStoreModel(QObject *parent, void*);
	//! @private
	void initStore(DataStore *store);

private Q_SLOTS:
	void storeChanged(int metaTypeId, const QString &key, bool wasDeleted);
	void storeResetted();

private:
	QScopedPointer<DataStoreModelPrivate> d;
};

// ------------- Generic Implementation -------------

template <typename T>
inline void DataStoreModel::setTypeId(bool resetColumns) {
	QTDATASYNC_STORE_ASSERT(T);
	setTypeId(qMetaTypeId<T>(), resetColumns);
}

template <typename T>
inline QModelIndex DataStoreModel::idIndex(const T &id) const {
	return idIndex(QVariant::fromValue<T>(id).toString());
}

template <typename T>
inline T DataStoreModel::key(const QModelIndex &index) const {
	return QVariant(key(index)).value<T>();
}

template <typename T>
inline T DataStoreModel::object(const QModelIndex &index) const {
	QTDATASYNC_STORE_ASSERT(T);
	Q_ASSERT_X(qMetaTypeId<T>() == typeId(), Q_FUNC_INFO, "object must be used with the stores typeId");
	return object(index).value<T>();
}

template <typename T>
T DataStoreModel::loadObject(const QModelIndex &index) const {
	QTDATASYNC_STORE_ASSERT(T);
	Q_ASSERT_X(qMetaTypeId<T>() == typeId(), Q_FUNC_INFO, "object must be used with the stores typeId");
	return loadObject(index).value<T>();
}

}

#endif // QTDATASYNC_DATASTOREMODEL_H
