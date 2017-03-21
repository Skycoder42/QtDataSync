#include "task.h"
#include "asyncdatastore.h"

#include <QtCore/QFutureWatcher>
#include <QtCore/qcoreapplication.h>

using namespace QtDataSync;

Task::Task(QFutureInterface<QVariant> d) :
	QFuture(&d)
{}

Task &Task::onResult(const std::function<void (QVariant)> &onSuccess, const std::function<void (QException &)> &onExcept, QObject *parent)
{
	auto watcher = new QFutureWatcher<QVariant>(parent ? parent : qApp);
	QObject::connect(watcher, &QFutureWatcherBase::finished, watcher, [=](){
		try {
			auto res = watcher->result();
			if(onSuccess)
				onSuccess(res);
		} catch (QException &e) {
			if(onExcept)
				onExcept(e);
			else {
				qCritical() << "Unhandelt exception in Task. Exception was:"
							<< e.what();
			}
		}
		watcher->deleteLater();
	});
	watcher->setFuture(*this);

	return *this;
}

GenericTask<void>::GenericTask(QFutureInterface<QVariant> d) :
	Task(d)
{}

GenericTask<void> &GenericTask<void>::onResult(const std::function<void ()> &onSuccess, const std::function<void (QException &)> &onExcept, QObject *parent)
{
	Task::onResult([=](QVariant){
		onSuccess();
	}, onExcept, parent);

	return *this;
}
