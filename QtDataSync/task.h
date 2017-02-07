#ifndef TASK_H
#define TASK_H

#include "qtdatasync_global.h"
#include <QFuture>
#include <functional>

namespace QtDataSync {
class AsyncDataStore;

class QTDATASYNCSHARED_EXPORT Task : public QFuture<QVariant>
{
	friend class AsyncDataStore;

public:
	void onResult(std::function<void(QVariant)> fn);
	void onException(std::function<void(QException)> fn);

protected:
	Task(AsyncDataStore *store, QFutureInterface<QVariant> d);
};

template <typename T>
class QTDATASYNCSHARED_EXPORT GenericTask : public Task
{
	friend class AsyncDataStore;

public:
	void onResult(std::function<void(T)> fn);

	T result() const;

private:
	GenericTask(AsyncDataStore *store, QFutureInterface<QVariant> d);
};

template <>
class QTDATASYNCSHARED_EXPORT GenericTask<void> : public Task
{
	friend class AsyncDataStore;

public:
	void onResult(std::function<void()> fn);

private:
	GenericTask(AsyncDataStore *store, QFutureInterface<QVariant> d);

	using QFuture<QVariant>::result;
};

template<typename T>
GenericTask<T>::GenericTask(AsyncDataStore *store, QFutureInterface<QVariant> d) :
	Task(store, d)
{}

template<typename T>
void GenericTask<T>::onResult(std::function<void(T)> fn)
{
	Task::onResult([=](QVariant result){
		fn(result.value<T>());
	});
}

template<typename T>
T GenericTask<T>::result() const
{
	return Task::result().template value<T>();
}

}

#endif // TASK_H
