#include "task.h"
#include "asyncdatastore.h"

#include <QFutureWatcher>
using namespace QtDataSync;

Task::Task(AsyncDataStore *store, QFutureInterface<QVariant> d) :
	QFuture(&d),
	_store(store)
{}

void Task::onResult(const std::function<void (QVariant)> &onSuccess, const std::function<void (QException)> &onExcept)
{
	auto watcher = new QFutureWatcher<QVariant>(_store);
	QObject::connect(watcher, &QFutureWatcherBase::finished, watcher, [=](){
		try {
			auto res = result();
			if(onSuccess)
				onSuccess(res);
		} catch (QException &e) {
			if(onExcept)
				onExcept(e);
			else {
				qWarning() << "Unhandelt exception:"
						   << e.what();
			}
		}
		watcher->deleteLater();
	});
}

GenericTask<void>::GenericTask(AsyncDataStore *store, QFutureInterface<QVariant> d) :
	Task(store, d)
{}

void GenericTask<void>::onResult(const std::function<void ()> &onSuccess, const std::function<void (QException)> &onExcept)
{
	Task::onResult([=](QVariant){
		onSuccess();
	}, onExcept);
}
