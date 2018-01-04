#ifndef CONFLICTRESOLVER_H
#define CONFLICTRESOLVER_H

#include <QtCore/qobject.h>
#include <QtCore/qscopedpointer.h>

#include <QtJsonSerializer/QJsonSerializer>

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/defaults.h"

namespace QtDataSync {

class ConflictResolverPrivate;
class Q_DATASYNC_EXPORT ConflictResolver : public QObject
{
	Q_OBJECT

public:
	explicit ConflictResolver(QObject *parent = nullptr);
	~ConflictResolver();

	void setDefaults(const Defaults &defaults);

	virtual QByteArray name() const;
	virtual QJsonObject resolveConflict(int typeId, const QJsonObject &data1, const QJsonObject &data2) const = 0;

protected:
	Defaults defaults() const;
	Logger *logger() const;
	QSettings *settings() const;
	const QJsonSerializer *serializer() const;

private:
	QScopedPointer<ConflictResolverPrivate> d;
};

template <typename T1, typename... Args>
class GenericConflictResolver : public GenericConflictResolver<Args...>
{
public:
	inline GenericConflictResolver(QObject *parent = nullptr) :
		GenericConflictResolver<Args...>(parent)
	{}

	virtual T1 resolveConflict(T1 data1, T1 data2) const = 0;

	inline QJsonObject resolveConflict(int typeId, const QJsonObject &data1, const QJsonObject &data2) const override {
		if(typeId == qMetaTypeId<T1>()) {
			const QJsonSerializer *ser = this->serializer();
			auto d1 = ser->deserialize<T1>(data1, const_cast<GenericConflictResolver<T1, Args...>*>(this));
			auto d2 = ser->deserialize<T1>(data2, const_cast<GenericConflictResolver<T1, Args...>*>(this));
			return ser->serialize<T1>(resolveConflict(d1, d2));
		} else
			return GenericConflictResolver<Args...>::resolveConflict(typeId, data1, data2);
	}
};

template <typename T1>
class GenericConflictResolver<T1> : public ConflictResolver
{
public:
	inline GenericConflictResolver(QObject *parent = nullptr) :
		ConflictResolver(parent)
	{}

	virtual T1 resolveConflict(T1 data1, T1 data2) const = 0;

	inline QJsonObject resolveConflict(int typeId, const QJsonObject &data1, const QJsonObject &data2) const override {
		if(typeId == qMetaTypeId<T1>()) {
			const QJsonSerializer *ser = this->serializer();
			auto d1 = ser->deserialize<T1>(data1, const_cast<GenericConflictResolver<T1>*>(this));
			auto d2 = ser->deserialize<T1>(data2, const_cast<GenericConflictResolver<T1>*>(this));
			return ser->serialize<T1>(resolveConflict(d1, d2));
		} else
			return QJsonObject();
	}
};

}

#endif // CONFLICTRESOLVER_H
