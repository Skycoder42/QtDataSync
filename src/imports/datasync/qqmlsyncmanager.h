#ifndef QQMLSYNCMANAGER_H
#define QQMLSYNCMANAGER_H

#include <QtCore/QObject>

#include <QtQml/QQmlParserStatus>
#include <QtQml/QJSValue>

#include <QtDataSync/syncmanager.h>

#ifdef DOXYGEN_RUN
namespace de::skycoder42::QtDataSync {

/*! @brief The QML binding of ::QtDataSync::SyncManager
 *
 * @since 4.0
 *
 * @sa QtDataSync::SyncManager
 */
class SyncManager : public ::QtDataSync::SyncManager
#else
namespace QtDataSync {

class QQmlSyncManager : public SyncManager, public QQmlParserStatus
#endif
{
	Q_OBJECT
	Q_DISABLE_COPY(QQmlSyncManager)
	Q_INTERFACES(QQmlParserStatus)

	/*! @brief Holds the name of the setup this manager operates on.
	 *
	 * @default{`QtDataSync::DefaultSetup`}
	 *
	 * Allows you to specify the name of the setup that this manager should use
	 *
	 * @warning This property must only ever be set on construction of the object
	 * and must not be changed afterwards. Do not set both, this property and node
	 *
	 * @accessors{
	 *	@memberAc{setupName}
	 *	@notifyAc{setupNameChanged()}
	 * }
	 *
	 * @sa ::QtDataSync::SyncManager::setupName, SyncManager::node, SyncManager::valid
	 */
	Q_PROPERTY(QString setupName READ setupName WRITE setSetupName NOTIFY setupNameChanged)
	/*! @brief Allows to directly set the remote object node to use
	 *
	 * @default{`nullptr`}
	 *
	 * Instead of a setup name, you can directly specify the remote object node via this property.
	 *
	 * @warning This property must only ever be set on construction of the object
	 * and must not be changed afterwards. Do not set both, this property and setupName
	 *
	 * @accessors{
	 *	@memberAc{node}
	 *	@notifyAc{nodeChanged()}
	 * }
	 *
	 * @sa SyncManager::setupName, SyncManager::valid
	 */
	Q_PROPERTY(QRemoteObjectNode* node READ node WRITE setNode NOTIFY nodeChanged)
	/*! @brief Specifies if the object was correctly initialized
	 *
	 * @default{`false`}
	 *
	 * Only becomes true if the given setup name resolves to a valid datasync
	 * instance or if the given node is valid
	 *
	 * @accessors{
	 *	@memberAc{valid}
	 *	@notifyAc{validChanged()}
	 *  @readonlyAc
	 * }
	 *
	 * @sa SyncManager::setupName, SyncManager::node
	 */
	Q_PROPERTY(bool valid READ valid NOTIFY validChanged)

public:
	//! @private
	explicit QQmlSyncManager(QObject *parent = nullptr);

	//! @private
	void classBegin() override;
	//! @private
	void componentComplete() override;

	//! @private
	QString setupName() const;
	//! @private
	QRemoteObjectNode* node() const;
	//! @private
	bool valid() const;

	//! @copydoc ::QtDataSync::SyncManager::runOnDownloaded(const std::function<void(SyncState)> &, bool)
	Q_INVOKABLE void runOnDownloaded(const QJSValue &resultFn, bool triggerSync = true);
	//! @copydoc ::QtDataSync::SyncManager::runOnSynchronized(const std::function<void(SyncState)> &, bool)
	Q_INVOKABLE void runOnSynchronized(const QJSValue &resultFn, bool triggerSync = true);

public Q_SLOTS:
	//! @private
	void setSetupName(QString setupName);
	//! @private
	void setNode(QRemoteObjectNode* node);

Q_SIGNALS:
	//! @notifyAcFn{AccountManager::setupName}
	void setupNameChanged(const QString &setupName);
	//! @notifyAcFn{AccountManager::node}
	void nodeChanged(QRemoteObjectNode* node);
	//! @notifyAcFn{AccountManager::valid}
	void validChanged(bool valid);

private:
	QString _setupName;
	QRemoteObjectNode *_node;
	bool _valid;
};

}

#endif // QQMLSYNCMANAGER_H
