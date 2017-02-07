#include "task.h"
using namespace QtDataSync;

Task::Task(AsyncDataStore *store, QFutureInterface<QVariant> d) :
	QFuture(&d)
{}

void Task::onResult(std::function<void (QVariant)> fn)
{

}

void Task::onException(std::function<void (QException)> fn)
{

}

QtDataSync::GenericTask<void>::GenericTask(AsyncDataStore *store, QFutureInterface<QVariant> d) :
	Task(store, d)
{}

void QtDataSync::GenericTask<void>::onResult(std::function<void ()> fn)
{
	Task::onResult([=](QVariant){
		fn();
	});
}
