#ifndef SINGLETASKQUEUE_H
#define SINGLETASKQUEUE_H

#include <QtCore/QObject>
#include <QtCore/QRunnable>
#include <QtCore/QThreadPool>
#include <QtCore/QMutex>
#include <QtCore/QQueue>
#include <QtCore/QSharedPointer>
#include <QtCore/QWeakPointer>

class SingleTaskQueue : public QObject
{
	Q_OBJECT

public:
	class Task : public QRunnable {
		friend class SingleTaskQueue;
	public:
		Task(SingleTaskQueue *queue);
		virtual void safeRun() = 0;
		void run() final;

	private:
		QWeakPointer<QMutex> _lockRef;
		SingleTaskQueue *_queue;
	};

	template<typename TFunction>
	class GenericTask : public Task {
	public:
		GenericTask(const TFunction &fn, SingleTaskQueue *queue);
		void safeRun() final;

	private:
		const TFunction _fn;
	};

	explicit SingleTaskQueue(QThreadPool *pool, QObject *parent = nullptr);
	~SingleTaskQueue();

	void enqueueTask(Task *task);
	template <typename TFunction>
	inline void enqueue(const TFunction &fn) {
		enqueueTask(new GenericTask<TFunction>(fn, this));
	}

	bool clear();
	bool isFinished() const;

private:
	QThreadPool *_pool;

	QSharedPointer<QMutex> _lock;
	QQueue<Task*> _tasks;

	void nextTask(QMutexLocker &lock);
	void completeTask(const Task * const task, QMutexLocker &lock);
};

template<typename TFunction>
SingleTaskQueue::GenericTask<TFunction>::GenericTask(const TFunction &fn, SingleTaskQueue *queue) :
	Task(queue),
	_fn(fn)
{}

template<typename TFunction>
void SingleTaskQueue::GenericTask<TFunction>::safeRun()
{
	_fn();
}

#endif // SINGLETASKQUEUE_H
