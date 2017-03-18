#include "mockstateholder.h"

MockStateHolder::MockStateHolder(QObject *parent) :
	StateHolder(parent),
	mutex(QMutex::Recursive),
	enabled(false),
	dummyReset(false),
	pseudoState()
{}

QtDataSync::StateHolder::ChangeHash MockStateHolder::listLocalChanges()
{
	QMutexLocker _(&mutex);
	if(!enabled)
		return {};
	else
		return pseudoState;
}

void MockStateHolder::markLocalChanged(const QtDataSync::ObjectKey &key, QtDataSync::StateHolder::ChangeState changed)
{
	QMutexLocker _(&mutex);
	if(enabled) {
		if(changed == Unchanged)
			pseudoState.remove(key);
		else
			pseudoState.insert(key, changed);
	}
}

QtDataSync::StateHolder::ChangeHash MockStateHolder::resetAllChanges(const QList<QtDataSync::ObjectKey> &changeKeys)
{
	QMutexLocker _(&mutex);
	if(enabled && ! dummyReset) {
		clearAllChanges();
		foreach (auto key, changeKeys)
			pseudoState.insert(key, Changed);
		return pseudoState;
	} else
		return pseudoState;
}

void MockStateHolder::clearAllChanges()
{
	QMutexLocker _(&mutex);
	if(enabled)
		pseudoState.clear();
}
