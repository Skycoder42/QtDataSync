#include "singletaskqueue.h"

#include <QtCore/QDebug>
#include <QtCore/QCoreApplication>

#define LOCK QMutexLocker _(_lock.data())

SingleTaskQueue::SingleTaskQueue(QThreadPool *pool, QObject *parent) :
	QObject(parent),
	_pool(pool),
	_lock(new QMutex()),
	_tasks()
{}

SingleTaskQueue::~SingleTaskQueue()
{
	if(!clear()) { //blocking wait for the queue to clear
		do {
			QThread::msleep(10);
			QCoreApplication::processEvents();
		} while(!isFinished());
	}
}

void SingleTaskQueue::enqueueTask(SingleTaskQueue::Task *task)
{
	LOCK;
	_tasks.enqueue(task);
	if(_tasks.size() == 1) //if this is the only/first task, start it
		nextTask(_);
}

bool SingleTaskQueue::clear()
{
	LOCK;
	if(!_tasks.isEmpty()) {
		//save the head, is still running
		auto head = _tasks.dequeue();

		//all the others: delete
		while(!_tasks.isEmpty()) {
			auto task = _tasks.dequeue();
			task->_queue = nullptr;
			task->_lockRef.clear();
			delete task;
		}

		//readd the head, as running tasks cannot be removed
		_tasks.enqueue(head);
	}

	return _tasks.isEmpty();
}

bool SingleTaskQueue::isFinished() const
{
	LOCK;
	return _tasks.isEmpty();
}

void SingleTaskQueue::nextTask(QMutexLocker &lock)
{
	if(!_tasks.isEmpty()) {
		auto task = _tasks.head();
		//explicit unlock of mutex to not include the (heavy) starting. mutex must not be relocked if used again
		lock.unlock();
		_pool->start(task);
	}
}

void SingleTaskQueue::completeTask(const SingleTaskQueue::Task * const task, QMutexLocker &lock)
{
	if(_tasks.dequeue() != task)
		qWarning() << "SingleTaskQueue: Completed task does not match the current queue head";
	nextTask(lock);
}



SingleTaskQueue::Task::Task(SingleTaskQueue *queue) :
	_lockRef(queue->_lock),
	_queue(queue)
{
	setAutoDelete(true);
}

void SingleTaskQueue::Task::run()
{
	safeRun();
	// done here instead of destructor, as it leads to a deadlock. See QTBUG-65486
	auto _lock = _lockRef.toStrongRef();
	if(_lock) {
		LOCK;
		if(_queue)
			_queue->completeTask(this, _);
	}
}
