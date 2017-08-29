#ifndef QTDATASYNC_DATASTOREMODEL_H
#define QTDATASYNC_DATASTOREMODEL_H

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/asyncdatastore.h"

#include <QtCore/qabstractitemmodel.h>

namespace QtDataSync {

class DataStoreModelPrivate;
class Q_DATASYNC_EXPORT DataStoreModel : public QAbstractListModel
{
	Q_OBJECT
	friend class DataStoreModelPrivate;

	Q_PROPERTY(int typeId READ typeId WRITE setTypeId)
	Q_PROPERTY(bool editable READ isEditable WRITE setEditable NOTIFY editableChanged)

public:
	explicit DataStoreModel(QObject *parent = nullptr);
	explicit DataStoreModel(const QString &setupName, QObject *parent = nullptr);
	explicit DataStoreModel(AsyncDataStore *store, QObject *parent = nullptr);
	~DataStoreModel();

	AsyncDataStore *store() const;

	int typeId() const;
	template <typename T>
	inline bool setTypeId();
	bool isEditable() const;

	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;

	QModelIndex idIndex(const QString &id) const;
	template <typename T>
	inline QModelIndex idIndex(const T &id) const;

	QString key(const QModelIndex &index) const;
	template <typename T>
	inline T key(const QModelIndex &index) const;

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role) override;

	QVariant object(const QModelIndex &index) const;
	template <typename T>
	inline T object(const QModelIndex &index) const;
	Task loadObject(const QModelIndex &index) const;
	template <typename T>
	GenericTask<T> loadObject(const QModelIndex &index) const;

	Qt::ItemFlags flags(const QModelIndex &index) const override;
	QHash<int, QByteArray> roleNames() const override;

public Q_SLOTS:
	bool setTypeId(int typeId);
	void setEditable(bool editable);

Q_SIGNALS:
	void storeLoaded();
	void storeError(const QException &exception);

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
