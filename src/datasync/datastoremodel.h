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

public:
	explicit DataStoreModel(QObject *parent = nullptr);
	explicit DataStoreModel(const QString &setupName, QObject *parent = nullptr);
	explicit DataStoreModel(AsyncDataStore *store, QObject *parent = nullptr);
	~DataStoreModel();

	AsyncDataStore *store() const;
	int typeId() const;

	bool setDataType(int typeId);
	template <typename T>
	inline bool setDataType();

	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	using QAbstractListModel::index;
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

	Qt::ItemFlags flags(const QModelIndex &index) const override;
	QHash<int, QByteArray> roleNames() const override;

Q_SIGNALS:
	void storeLoaded();

	void storeError(const QException &exception);

private Q_SLOTS:
	void storeChanged(int metaTypeId, const QString &key, bool wasDeleted);
	void storeResetted();

private:
	QScopedPointer<DataStoreModelPrivate> d;
};

template <typename T>
inline bool DataStoreModel::setDataType() {
	return setDataType(qMetaTypeId<T>());
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
	return object(index).value<T>();
}

}

#endif // QTDATASYNC_DATASTOREMODEL_H
