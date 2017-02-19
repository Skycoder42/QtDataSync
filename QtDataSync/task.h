#ifndef TASK_H
#define TASK_H

#include "qtdatasync_global.h"
#include <QFuture>
#include <QPointer>
#include <functional>

namespace QtDataSync {
class AsyncDataStore;

template <typename T>
class GenericTask;

class QTDATASYNCSHARED_EXPORT Task : public QFuture<QVariant>
{
	friend class AsyncDataStore;

public:
	Task &onResult(const std::function<void(QVariant)> &onSuccess, const std::function<void(QException &)> &onExcept = {});

	template <typename T>
	GenericTask<T> toGeneric() const;

protected:
	Task(AsyncDataStore *store, QFutureInterface<QVariant> d);

private:
	QPointer<AsyncDataStore> _store;
};

template <typename T>
class QTDATASYNCSHARED_EXPORT GenericTask : public Task
{
	friend class AsyncDataStore;
	friend class Task;

public:
	GenericTask<T> &onResult(const std::function<void(T)> &onSuccess, const std::function<void(QException &)> &onExcept = {});

	T result() const;

private:
	GenericTask(AsyncDataStore *store, QFutureInterface<QVariant> d);
};

template <>
class QTDATASYNCSHARED_EXPORT GenericTask<void> : public Task
{
	friend class AsyncDataStore;
	friend class Task;

public:
	GenericTask<void> &onResult(const std::function<void()> &onSuccess, const std::function<void(QException &)> &onExcept = {});

private:
	GenericTask(AsyncDataStore *store, QFutureInterface<QVariant> d);

	using QFuture<QVariant>::result;
};

// ------------- Generic Implementation -------------

template<typename T>
GenericTask<T> Task::toGeneric() const
{
	return GenericTask<T>(_store, d);
}

template<typename T>
GenericTask<T>::GenericTask(AsyncDataStore *store, QFutureInterface<QVariant> d) :
	Task(store, d)
{}

template<typename T>
GenericTask<T> &GenericTask<T>::onResult(const std::function<void (T)> &onSuccess, const std::function<void (QException &)> &onExcept)
{
	Task::onResult([=](QVariant result){
		onSuccess(result.value<T>());
	}, onExcept);

	return *this;
}

template<typename T>
T GenericTask<T>::result() const
{
	return Task::result().template value<T>();
}

}

#endif // TASK_H
