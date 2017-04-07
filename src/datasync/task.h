#ifndef QTDATASYNC_TASK_H
#define QTDATASYNC_TASK_H

#include "QtDataSync/qdatasync_global.h"

#include <QtCore/qfuture.h>
#include <QtCore/qpointer.h>
#include <QtCore/qexception.h>

#include <functional>

namespace QtDataSync {
class AsyncDataStore;

template <typename T>
class GenericTask;

//! A class to extend QFuture by an onResult handler
class Q_DATASYNC_EXPORT Task : public QFuture<QVariant>
{
	friend class AsyncDataStore;

public:
	//! @copybrief Task::onResult(const std::function<void(QVariant)> &, const std::function<void(const QException &)> &)
	Task &onResult(QObject *parent,
				   const std::function<void(QVariant)> &onSuccess,
				   const std::function<void(const QException &)> &onExcept = {});
	//! Set a handler to be called when the future has finished
	Task &onResult(const std::function<void(QVariant)> &onSuccess,
				   const std::function<void(const QException &)> &onExcept = {});

	//! Converts this task to a generic task of the given type
	template <typename T>
	GenericTask<T> toGeneric() const;

protected:
	//! Constructor with future interface
	Task(QFutureInterface<QVariant> d);
};

//! Generic version of the Task
template <typename T>
class GenericTask : public Task
{
	friend class AsyncDataStore;
	friend class Task;

public:
	//! Constructor with future interface
	GenericTask(QFutureInterface<QVariant> d = {});

	//! @copydoc Task::onResult(QObject *, const std::function<void(QVariant)> &, const std::function<void(const QException &)> &)
	GenericTask<T> &onResult(QObject *parent,
							 const std::function<void(T)> &onSuccess,
							 const std::function<void(const QException &)> &onExcept = {});
	//! @copydoc Task::onResult(const std::function<void(QVariant)> &, const std::function<void(const QException &)> &)
	GenericTask<T> &onResult(const std::function<void(T)> &onSuccess,
							 const std::function<void(const QException &)> &onExcept = {});

	//! Returns the tasks result
	T result() const;
};

//! Generic version of the Task, specialization for void
template <>
class Q_DATASYNC_EXPORT GenericTask<void> : public Task
{
	friend class AsyncDataStore;
	friend class Task;

public:
	//! @copydoc GenericTask::GenericTask
	GenericTask(QFutureInterface<QVariant> d = {});

	//! @copydoc GenericTask::onResult(QObject *, const std::function<void(QVariant)> &, const std::function<void(const QException &)> &)
	GenericTask<void> &onResult(QObject *parent,
								const std::function<void()> &onSuccess,
								const std::function<void(const QException &)> &onExcept = {});
	//! @copydoc GenericTask::onResult(const std::function<void(QVariant)> &, const std::function<void(const QException &)> &)
	GenericTask<void> &onResult(const std::function<void()> &onSuccess,
								const std::function<void(const QException &)> &onExcept = {});

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
GenericTask<T> &GenericTask<T>::onResult(QObject *parent, const std::function<void (T)> &onSuccess, const std::function<void (const QException &)> &onExcept)
{
	Task::onResult(parent, [=](QVariant result){
		onSuccess(result.value<T>());
	}, onExcept);

	return *this;
}

template<typename T>
GenericTask<T> &GenericTask<T>::onResult(const std::function<void (T)> &onSuccess, const std::function<void (const QException &)> &onExcept)
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

#endif // QTDATASYNC_TASK_H
