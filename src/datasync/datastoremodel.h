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
	inline bool setDataType() {
		return setDataType(qMetaTypeId<T>());
	}

	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	using QAbstractListModel::index;
	QModelIndex idIndex(const QString &id) const;
	template <typename T>
	inline QModelIndex idIndex(const T &id) const {
		return idIndex(QVariant::fromValue<T>(id).toString());
	}
	template <typename T = QString>
	inline T key(const QModelIndex &index) const {
		return QVariant(keyImpl(index)).value<T>();
	}

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role) override;

	Qt::ItemFlags flags(const QModelIndex &index) const override;
	QHash<int, QByteArray> roleNames() const override;

Q_SIGNALS:
	void storeLoaded();

private Q_SLOTS:
	void storeChanged(int metaTypeId, const QString &key, bool wasDeleted);
	void storeResetted();

private:
	QScopedPointer<DataStoreModelPrivate> d;

	QString keyImpl(const QModelIndex &index) const;
};

}

#endif // QTDATASYNC_DATASTOREMODEL_H
