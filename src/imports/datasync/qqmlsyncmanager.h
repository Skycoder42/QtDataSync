#ifndef QQMLSYNCMANAGER_H
#define QQMLSYNCMANAGER_H

#include <QtCore/QObject>

#include <QtQml/QQmlParserStatus>
#include <QtQml/QJSValue>

#include <QtDataSync/syncmanager.h>

namespace QtDataSync {

class QQmlSyncManager : public SyncManager, public QQmlParserStatus
{
	Q_OBJECT
	Q_DISABLE_COPY(QQmlSyncManager)
	Q_INTERFACES(QQmlParserStatus)

	Q_PROPERTY(QString setupName READ setupName WRITE setSetupName NOTIFY setupNameChanged)
	Q_PROPERTY(QRemoteObjectNode* node READ node WRITE setNode NOTIFY nodeChanged)
	Q_PROPERTY(bool valid READ valid NOTIFY validChanged)

public:
	explicit QQmlSyncManager(QObject *parent = nullptr);

	void classBegin() override;
	void componentComplete() override;

	QString setupName() const;
	QRemoteObjectNode* node() const;
	bool valid() const;

	Q_INVOKABLE void runOnDownloaded(QJSValue resultFn, bool triggerSync = true);
	Q_INVOKABLE void runOnSynchronized(QJSValue resultFn, bool triggerSync = true);

public Q_SLOTS:
	void setSetupName(QString setupName);
	void setNode(QRemoteObjectNode* node);

Q_SIGNALS:
	void setupNameChanged(QString setupName);
	void nodeChanged(QRemoteObjectNode* node);
	void validChanged(bool valid);

private:
	QString _setupName;
	QRemoteObjectNode *_node;
	bool _valid;
};

}

#endif // QQMLSYNCMANAGER_H
