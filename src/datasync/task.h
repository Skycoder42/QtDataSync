#ifndef QTDATASYNC_TASK_H
#define QTDATASYNC_TASK_H

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/exceptions.h"

#include <QtCore/qfuture.h>
#include <QtCore/qpointer.h>
#include <QtCore/qexception.h>
#include <QtCore/qmutex.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qscopedpointer.h>
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
	Task(QFutureInterface<QVariant> d); //MAJOR make public
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
//! Generic Task that updates an existing dataset instead of creating a new one (objects only)
class UpdateTask : public Task
{
	static_assert(std::is_base_of<QObject, typename std::remove_pointer<T>::type>::value, "T must inherit QObject");

	friend class AsyncDataStore;

public:
	//! Constructor with future interface and the dataset to be updated
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
	struct UpdateData {
		T data;
		bool updated;
		QScopedPointer<QMutex> mutex;
	};

	QSharedPointer<UpdateData> _data;

	static T interalGet(QSharedPointer<UpdateData> data, QVariant result);
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
	Task::onResult(parent, [onSuccess](QVariant result){
		onSuccess(result.value<T>());
	}, onExcept);

	return *this;
}

template<typename T>
GenericTask<T> &GenericTask<T>::onResult(const std::function<void (T)> &onSuccess, const std::function<void (const QException &)> &onExcept)
{
	Task::onResult([onSuccess](QVariant result){
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
	_data(new UpdateData())
{
	Q_ASSERT_X(data, Q_FUNC_INFO, "UpdateTask must have an existing data object to update!");
	_data->data = data;
	_data->updated = false;
	_data->mutex.reset(new QMutex());
}

template<typename T>
UpdateTask<T> &UpdateTask<T>::onResult(QObject *parent, const std::function<void (T)> &onSuccess, const std::function<void (const QException &)> &onExcept)
{
	auto data = _data;
	Task::onResult(parent, [onSuccess, data](QVariant res){
		onSuccess(interalGet(data, res));
	}, onExcept);

	return *this;
}

template<typename T>
UpdateTask<T> &UpdateTask<T>::onResult(const std::function<void (T)> &onSuccess, const std::function<void (const QException &)> &onExcept)
{
	auto data = _data;
	Task::onResult([onSuccess, data](QVariant res){
		onSuccess(interalGet(data, res));
	}, onExcept);

	return *this;
}

template<typename T>
T UpdateTask<T>::result() const
{
	return interalGet(_data, Task::result());
}

template<typename T>
T UpdateTask<T>::interalGet(QSharedPointer<UpdateData> data, QVariant result)
{
	data->mutex->lock();
	if(!data->updated) {
		try {
			auto newResult = result.template value<T>();
			auto oldMeta = data->data->metaObject();
			auto newMeta = newResult->metaObject();

			//verify objects match
			if(!newMeta->inherits(oldMeta))
				throw DataSyncException("loadInto: Loaded class type does not match the one to load into!");
			auto user = oldMeta->userProperty();
			if(user.read(newResult).toString() != user.read(data->data).toString())
				throw DataSyncException("loadInto: The id of the loaded data does not match the one to be updated!");

			//pass on properties
			for(auto i = 1; i < oldMeta->propertyCount(); i++) {//skip object name
				auto prop = oldMeta->property(i);
				if(prop.isStored())
					prop.write(data->data, prop.read(newResult));
			}
			foreach(auto dynProp, newResult->dynamicPropertyNames())
				data->data->setProperty(dynProp, newResult->property(dynProp));

			data->updated = true;
			newResult->deleteLater();
		} catch(...) {
			data->mutex->unlock();
			throw;
		}
	}
	data->mutex->unlock();

	return data->data;
}

}

#endif // QTDATASYNC_TASK_H
