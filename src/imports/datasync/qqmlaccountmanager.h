#ifndef QQMLACCOUNTMANAGER_H
#define QQMLACCOUNTMANAGER_H

#include <QtCore/QObject>

#include <QtQml/QJSValue>
#include <QtQml/QQmlParserStatus>

#include <QtDataSync/accountmanager.h>

#ifdef DOXYGEN_RUN
namespace de::skycoder42::QtDataSync {

/*! @brief The QML binding of ::QtDataSync::AccountManager
 *
 * @since 4.0
 *
 * @sa QtDataSync::AccountManager
 */
class AccountManager : public ::QtDataSync::AccountManager
#else
namespace QtDataSync {

class QQmlAccountManager : public AccountManager, public QQmlParserStatus
#endif
{
	Q_OBJECT
	Q_DISABLE_COPY(QQmlAccountManager)
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
	 * @sa ::QtDataSync::AccountManager::setupName, AccountManager::node, AccountManager::valid
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
	 * @sa AccountManager::setupName, AccountManager::valid
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
	 * @sa AccountManager::setupName, AccountManager::node
	 */
	Q_PROPERTY(bool valid READ valid NOTIFY validChanged)

public:
	//! @private
	explicit QQmlAccountManager(QObject *parent = nullptr);

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

	//! @copydoc ::QtDataSync::AccountManager::isTrustedImport(const QJsonObject &)
	Q_INVOKABLE bool isTrustedImport(const QJsonObject &importData) const;

	//! @copydoc ::QtDataSync::AccountManager::exportAccount(bool, const std::function<void(QJsonObject)> &, const std::function<void(QString)> &)
	Q_INVOKABLE void exportAccount(bool includeServer, const QJSValue &completedFn, const QJSValue &errorFn = {});
	//! @copydoc ::QtDataSync::AccountManager::exportAccountTrusted(bool, const QString &, const std::function<void(QJsonObject)> &, const std::function<void(QString)> &)
	Q_INVOKABLE void exportAccountTrusted(bool includeServer, const QString &password, const QJSValue &completedFn, const QJSValue &errorFn = {});
	//! @copydoc ::QtDataSync::AccountManager::importAccount(const QJsonObject &, const std::function<void(bool,QString)> &, bool)
	Q_INVOKABLE void importAccount(const QJsonObject &importData, const QJSValue &completedFn, bool keepData = false);
	//! @copydoc ::QtDataSync::AccountManager::importAccountTrusted(const QJsonObject &, const QString &, const std::function<void(bool,QString)> &, bool)
	Q_INVOKABLE void importAccountTrusted(const QJsonObject &importData, const QString &password, const QJSValue &completedFn, bool keepData = false);

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
	QRemoteObjectNode* _node;
	bool _valid;
};

}

#endif // QQMLACCOUNTMANAGER_H
