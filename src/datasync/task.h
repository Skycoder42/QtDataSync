#ifndef QTDATASYNC_TASK_H
#define QTDATASYNC_TASK_H

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/exceptions.h"

#include <QtCore/qfuture.h>
#include <QtCore/qpointer.h>
#include <QtCore/qexception.h>
#include <QtCore/qmutex.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qmetaobject.h>

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

template <typename T>
class UpdateTask : public Task
{
	static_assert(std::is_base_of<QObject, typename std::remove_pointer<T>::type>::value, "T must inherit QObject");

	friend class AsyncDataStore;

public:
	//! Constructor with future interface
	UpdateTask(T data, QFutureInterface<QVariant> d = {});

	//! @copydoc Task::onResult(QObject *, const std::function<void(QVariant)> &, const std::function<void(const QException &)> &)
	UpdateTask<T> &onResult(QObject *parent,
							 const std::function<void(T)> &onSuccess,
							 const std::function<void(const QException &)> &onExcept = {});
	//! @copydoc Task::onResult(const std::function<void(QVariant)> &, const std::function<void(const QException &)> &)
	UpdateTask<T> &onResult(const std::function<void(T)> &onSuccess,
							 const std::function<void(const QException &)> &onExcept = {});

	//! Returns the updated tasks result
	T result() const;

private:
	T _data;
	bool _updated;
	QSharedPointer<QMutex> _mutex;

	T interalGet();
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

// ------------- Update Task Implementation -------------

template<typename T>
UpdateTask<T>::UpdateTask(T data, QFutureInterface<QVariant> d) :
	Task(d),
	_data(data),
	_updated(false),
	_mutex(new QMutex())
{
	Q_ASSERT_X(_data, Q_FUNC_INFO, "UpdateTask must have an existing data object to update!");
}

template<typename T>
UpdateTask<T> &UpdateTask<T>::onResult(QObject *parent, const std::function<void (T)> &onSuccess, const std::function<void (const QException &)> &onExcept)
{
	Task::onResult(parent, [=](QVariant){
		onSuccess(interalGet());
	}, onExcept);

	return *this;
}

template<typename T>
UpdateTask<T> &UpdateTask<T>::onResult(const std::function<void (T)> &onSuccess, const std::function<void (const QException &)> &onExcept)
{
	Task::onResult([=](QVariant){
		onSuccess(interalGet());
	}, onExcept);

	return *this;
}

template<typename T>
T UpdateTask<T>::result() const
{
	return interalGet();
}

template<typename T>
T UpdateTask<T>::interalGet()
{
	_mutex->lock();
	if(!_updated) {
		try {
			auto newResult = Task::result().template value<T>();
			auto oldMeta = _data->metaObject();
			auto newMeta = newResult->metaObject();

			//verify objects match
			if(!newMeta->inherits(oldMeta))
				throw DataSyncException("loadInto: Loaded class type does not match the one to load into!");
			auto user = oldMeta->userProperty();
			if(user.read(newResult).toString() != user.read(_data).toString())
				throw DataSyncException("loadInto: The id of the loaded data does not match the one to be updated!");

			//pass on properties
			for(auto i = 1; i < oldMeta->propertyCount(); i++) {//skip object name
				auto prop = oldMeta->property(i);
				if(prop.isStored())
					prop.write(_data, prop.read(newResult));
			}
			foreach(auto dynProp, newResult->dynamicPropertyNames())
				_data->setProperty(dynProp, newResult->property(dynProp));

			_updated = true;
			newResult->deleteLater();
		} catch(...) {
			_mutex->unlock();
			throw;
		}
	}
	_mutex->unlock();

	return _data;
}

}

#endif // QTDATASYNC_TASK_H
