#include "task.h"
#include "asyncdatastore.h"

#include <QtCore/QFutureWatcher>
#include <QtCore/qcoreapplication.h>

using namespace QtDataSync;

Task::Task(QFutureInterface<QVariant> d) :
	QFuture(&d)
{}

Task &Task::onResult(QObject *parent, const std::function<void (QVariant)> &onSuccess, const std::function<void (QException &)> &onExcept)
{
	auto watcher = new QFutureWatcher<QVariant>(parent);
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

Task &Task::onResult(const std::function<void (QVariant)> &onSuccess, const std::function<void (QException &)> &onExcept)
{
	return onResult(qApp, onSuccess, onExcept);
}

GenericTask<void>::GenericTask(QFutureInterface<QVariant> d) :
	Task(d)
{}

GenericTask<void> &GenericTask<void>::onResult(QObject *parent, const std::function<void ()> &onSuccess, const std::function<void (QException &)> &onExcept)
{
	Task::onResult(parent, [=](QVariant){
		onSuccess();
	}, onExcept);

	return *this;
}

GenericTask<void> &GenericTask<void>::onResult(const std::function<void ()> &onSuccess, const std::function<void (QException &)> &onExcept)
{
	Task::onResult([=](QVariant){
		onSuccess();
	}, onExcept);

	return *this;
}
