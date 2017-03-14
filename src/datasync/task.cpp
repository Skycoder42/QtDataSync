#include "task.h"
#include "asyncdatastore.h"

#include <QtCore/QFutureWatcher>

using namespace QtDataSync;

Task::Task(AsyncDataStore *store, QFutureInterface<QVariant> d) :
	QFuture(&d),
	_store(store)
{}

Task &Task::onResult(const std::function<void (QVariant)> &onSuccess, const std::function<void (QException &)> &onExcept)
{
	auto watcher = new QFutureWatcher<QVariant>(_store);
	QObject::connect(watcher, &QFutureWatcherBase::finished, watcher, [=](){
		try {
			watcher->result();
			auto res = watcher->result();
			if(onSuccess)
				onSuccess(res);
		} catch (QException &e) {
			if(onExcept)
				onExcept(e);
			else {
				qCritical() << "Unhandelt exception in Task:"
							<< e.what();
			}
		}
		watcher->deleteLater();
	});
	watcher->setFuture(*this);

	return *this;
}

GenericTask<void>::GenericTask(AsyncDataStore *store, QFutureInterface<QVariant> d) :
	Task(store, d)
{}

GenericTask<void> &GenericTask<void>::onResult(const std::function<void ()> &onSuccess, const std::function<void (QException &)> &onExcept)
{
	Task::onResult([=](QVariant){
		onSuccess();
	}, onExcept);

	return *this;
}
