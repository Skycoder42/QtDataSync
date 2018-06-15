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

	//! @readAcFn{DataStoreModel::typeId}
	int typeId() const;
	//! @writeAcFn{DataStoreModel::typeId}
	template <typename T>
	inline void setTypeId(bool resetColumns = true);
	//! @readAcFn{DataStoreModel::editable}
	bool isEditable() const;

	//! @inherit{QAbstractListModel::headerData}
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	//! @inherit{QAbstractListModel::rowCount}
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent) const override;
	//! @inherit{QAbstractListModel::canFetchMore}
	bool canFetchMore(const QModelIndex &parent) const override;
	//! @inherit{QAbstractListModel::fetchMore}
	void fetchMore(const QModelIndex &parent) override;

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

	//! @inherit{QAbstractListModel::data}
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	//! @inherit{QAbstractListModel::setData}
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

	//! @inherit{QAbstractListModel::flags}
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	//! @inherit{QAbstractListModel::roleNames}
	QHash<int, QByteArray> roleNames() const override;

	int addColumn(const QString &text);
	int addColumn(const QString &text, const char *propertyName);
	void addRole(int column, int role, const char *propertyName);

public Q_SLOTS:
	//! @writeAcFn{DataStoreModel::typeId}
	void setTypeId(int typeId); //MAJOR merge methods
	void setTypeId(int typeId, bool resetColumns);
	//! @writeAcFn{DataStoreModel::editable}
	void setEditable(bool editable);

	//! Reloads all data in the model
	void reload();

Q_SIGNALS:
	//! Emitted when the underlying DataStore throws an exception
	void storeError(const QException &exception, QPrivateSignal);
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
