#ifndef MOCKSTATEHOLDER_H
#define MOCKSTATEHOLDER_H

#include <QMutex>
#include <QObject>
#include <stateholder.h>

class MockStateHolder : public QtDataSync::StateHolder
{
	Q_OBJECT

public:
	explicit MockStateHolder(QObject *parent = nullptr);

	ChangeHash listLocalChanges() override;
	void markLocalChanged(const QtDataSync::ObjectKey &key, ChangeState changed) override;
	ChangeHash resetAllChanges(const QList<QtDataSync::ObjectKey> &changeKeys) override;
	void clearAllChanges() override;

public:
	QMutex mutex;
	bool enabled;
	bool dummyReset;
	ChangeHash pseudoState;
};

#endif // MOCKSTATEHOLDER_H
