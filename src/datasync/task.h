#ifndef TASK_H
#define TASK_H

#include "QtDataSync/qdatasync_global.h"

#include <QtCore/qfuture.h>
#include <QtCore/qpointer.h>

#include <functional>

namespace QtDataSync {
class AsyncDataStore;

template <typename T>
class GenericTask;

class Q_DATASYNC_EXPORT Task : public QFuture<QVariant>
{
	friend class AsyncDataStore;

public:
	Task &onResult(const std::function<void(QVariant)> &onSuccess, const std::function<void(QException &)> &onExcept = {});

	template <typename T>
	GenericTask<T> toGeneric() const;

protected:
	Task(QFutureInterface<QVariant> d);
};

template <typename T>
class GenericTask : public Task
{
	friend class AsyncDataStore;
	friend class Task;

public:
	GenericTask(QFutureInterface<QVariant> d = {});

	GenericTask<T> &onResult(const std::function<void(T)> &onSuccess, const std::function<void(QException &)> &onExcept = {});

	T result() const;
};

template <>
class Q_DATASYNC_EXPORT GenericTask<void> : public Task
{
	friend class AsyncDataStore;
	friend class Task;

public:
	GenericTask(QFutureInterface<QVariant> d = {});

	GenericTask<void> &onResult(const std::function<void()> &onSuccess, const std::function<void(QException &)> &onExcept = {});

private:
	using QFuture<QVariant>::result;
};

// ------------- Generic Implementation -------------

template<typename T>
GenericTask<T> Task::toGeneric() const
{
	return GenericTask<T>(d);
}

template<typename T>
GenericTask<T>::GenericTask(QFutureInterface<QVariant> d) :
	Task(d)
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
