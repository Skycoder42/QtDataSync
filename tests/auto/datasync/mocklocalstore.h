#ifndef MOCKLOCALSTORE_H
#define MOCKLOCALSTORE_H

#include <QMutex>
#include <QObject>
#include <localstore.h>

class MockLocalStore : public QtDataSync::LocalStore
{
	Q_OBJECT

public:
	explicit MockLocalStore(QObject *parent = nullptr);

	QList<QtDataSync::ObjectKey> loadAllKeys() override;
	void resetStore() override;

public slots:
	void count(quint64 id, const QByteArray &typeName) override;
	void keys(quint64 id, const QByteArray &typeName) override;
	void loadAll(quint64 id, const QByteArray &typeName) override;
	void load(quint64 id, const QtDataSync::ObjectKey &key, const QByteArray &keyProperty) override;
	void save(quint64 id, const QtDataSync::ObjectKey &key, const QJsonObject &object, const QByteArray &keyProperty) override;
	void remove(quint64 id, const QtDataSync::ObjectKey &key, const QByteArray &keyProperty) override;
	void search(quint64 id, const QByteArray &typeName, const QString &searchQuery) override;

public:
	QMutex mutex;
	bool enabled;
	QHash<QtDataSync::ObjectKey, QJsonObject> pseudoStore;
	int failCount;
};

#endif // MOCKLOCALSTORE_H
