#ifndef QTDATASYNC_DATASTOREMODEL_H
#define QTDATASYNC_DATASTOREMODEL_H

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/asyncdatastore.h"

#include <QtCore/qabstractitemmodel.h>

namespace QtDataSync {

class DataStoreModelPrivate;
//! A passive item model for a datasync data store
class Q_DATASYNC_EXPORT DataStoreModel : public QAbstractListModel
{
	Q_OBJECT
	friend class DataStoreModelPrivate;

	//! Holds the type the model loads data for
	Q_PROPERTY(int typeId READ typeId WRITE setTypeId)
	//! Specifies whether the model items can be edited
	Q_PROPERTY(bool editable READ isEditable WRITE setEditable NOTIFY editableChanged)

public:
	//! Constructs a model for the default setup
	explicit DataStoreModel(QObject *parent = nullptr);
	//! Constructs a model for the given setup
	explicit DataStoreModel(const QString &setupName, QObject *parent = nullptr);
	//! Constructs a model on the given store
	explicit DataStoreModel(AsyncDataStore *store, QObject *parent = nullptr);
	//! Destructor
	~DataStoreModel();

	//! Returns the data store the model works on
	AsyncDataStore *store() const;

	//! @readAcFn{DataStoreModel::typeId}
	int typeId() const;
	//! @writeAcFn{DataStoreModel::typeId}
	template <typename T>
	inline bool setTypeId();
	//! @readAcFn{DataStoreModel::editable}
	bool isEditable() const;

	//! @inherit{QAbstractListModel::headerData}
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	//! @inherit{QAbstractListModel::rowCount}
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;

	//! Returns the index of the item with the given id
	QModelIndex idIndex(const QString &id) const;
	//! Returns the index of the item with the given id
	template <typename T>
	inline QModelIndex idIndex(const T &id) const;

	//! Returns the key of the item at the given index
	QString key(const QModelIndex &index) const;
	//! Returns the key of the item at the given index
	template <typename T>
	inline T key(const QModelIndex &index) const;

	//! @inherit{QAbstractListModel::data}
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	//! @inherit{QAbstractListModel::setData}
	bool setData(const QModelIndex &index, const QVariant &value, int role) override;

	/*!
	Returns the object at the given index

	@param index The model index to return the object for
	@returns The data at the index, or an invalid variant

	@warning When working with QObjects, the method is potentially dangerous, as the
	returned object is owend by the model, and can be deleted any time. It is fine
	to use the returned object in a local scope. Do not leave a local scope,
	or use QPointer to be able to react in case the object gets deleted. To get
	a copy that you own, use the loadObject() method.

	@sa DataStoreModel::loadObject
	*/
	QVariant object(const QModelIndex &index) const;
	/*!
	Returns the object at the given index

	@tparam The type of object to return. Must match DataStoreModel::typeId
	@param index The model index to return the object for
	@returns The data at the index, or an invalid variant

	@warning When working with QObjects, the method is potentially dangerous, as the
	returned object is owend by the model, and can be deleted any time. It is fine
	to use the returned object in a local scope. Do not leave a local scope,
	or use QPointer to be able to react in case the object gets deleted. To get
	a copy that you own, use the loadObject() method.

	@sa DataStoreModel::loadObject
	*/
	template <typename T>
	inline T object(const QModelIndex &index) const;
	//! Loads the object at the given index from the store via AsyncDataStore::load
	Task loadObject(const QModelIndex &index) const;
	//! Loads the object at the given index from the store via AsyncDataStore::load
	template <typename T>
	GenericTask<T> loadObject(const QModelIndex &index) const;

	//! @inherit{QAbstractListModel::flags}
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	//! @inherit{QAbstractListModel::roleNames}
	QHash<int, QByteArray> roleNames() const override;

public Q_SLOTS:
	//! @writeAcFn{DataStoreModel::typeId}
	bool setTypeId(int typeId);
	//! @writeAcFn{DataStoreModel::editable}
	void setEditable(bool editable);

Q_SIGNALS:
	//! Emitted when the model successfully loaded the current state from the store, and thus is "ready"
	void storeLoaded();
	//! Emitted when the store throws an exception
	void storeError(const QException &exception);

	//! @notifyAcFn{DataStoreModel::editable}
	void editableChanged(bool editable);

private Q_SLOTS:
	void storeChanged(int metaTypeId, const QString &key, bool wasDeleted);
	void storeResetted();

private:
	QScopedPointer<DataStoreModelPrivate> d;
};

// ------------- Generic Implementation -------------

template <typename T>
inline bool DataStoreModel::setTypeId() {
	return setTypeId(qMetaTypeId<T>());
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
	Q_ASSERT_X(qMetaTypeId<T>() == typeId(), Q_FUNC_INFO, "object must be used with the stores typeId");
	return object(index).value<T>();
}

template <typename T>
GenericTask<T> DataStoreModel::loadObject(const QModelIndex &index) const {
	Q_ASSERT_X(qMetaTypeId<T>() == typeId(), Q_FUNC_INFO, "loadObject must be used with the stores typeId");
	auto iKey = key(index);
	if(iKey.isNull()) {
		QFutureInterface<QVariant> d;
		d.reportStarted();
		d.reportException(DataSyncException("QModelIndex is not valid"));
		d.reportFinished();
		return GenericTask<T>(d);
	} else
		return store()->load<T>(iKey);
}

}

#endif // QTDATASYNC_DATASTOREMODEL_H
