#ifndef QQMLUSEREXCHANGEMANAGER_H
#define QQMLUSEREXCHANGEMANAGER_H

#include <QtCore/QObject>

#include <QtQml/QQmlParserStatus>

#include <QtDataSync/userexchangemanager.h>

#include "qqmlaccountmanager.h"

#ifdef DOXYGEN_RUN
namespace de::skycoder42::QtDataSync {

/*! @brief The QML binding of ::QtDataSync::UserExchangeManager
 *
 * @since 4.0
 *
 * @sa QtDataSync::UserExchangeManager
 */
class UserExchangeManager : public ::QtDataSync::UserExchangeManager
#else
namespace QtDataSync {

class QQmlUserExchangeManager : public UserExchangeManager, public QQmlParserStatus
#endif
{
	Q_OBJECT
	Q_DISABLE_COPY(QQmlUserExchangeManager)
	Q_INTERFACES(QQmlParserStatus)

	/*! @brief The default port (13742) for the data exchange
	 *
	 * @default{`13742`}
	 *
	 * @accessors{
	 *	@memberAc{DataExchangePort}
	 *  @readonlyAc
	 * }
	 *
	 * @sa ::QtDataSync::UserExchangeManager::DataExchangePort
	 */
	Q_PROPERTY(quint16 DataExchangePort READ DataExchangePort CONSTANT)
	/*! @brief Holds the name of the setup this manager operates on.
	 *
	 * @default{`QtDataSync::DefaultSetup`}
	 *
	 * Allows you to specify the name of the setup that this manager should use
	 *
	 * @warning This property must only ever be set on construction of the object
	 * and must not be changed afterwards. Do not set both, this property and manager
	 *
	 * @accessors{
	 *	@memberAc{setupName}
	 *	@notifyAc{setupNameChanged()}
	 * }
	 *
	 * @sa ::QtDataSync::UserExchangeManager::setupName, UserExchangeManager::manager,
	 * UserExchangeManager::valid
	 */
	Q_PROPERTY(QString setupName READ setupName WRITE setSetupName NOTIFY setupNameChanged)
	/*! @brief Holds the data store this model operates on
	 *
	 * @default{`nullptr`}
	 *
	 * Allows you to specify a datastore instead of a setup to initialize the model
	 *
	 * @warning This property must only ever be set on construction of the object
	 * and must not be changed afterwards. Do not set both, this property and setupName
	 *
	 * @accessors{
	 *	@memberAc{setupName}
	 *	@notifyAc{setupNameChanged()}
	 * }
	 *
	 * @sa UserExchangeManager::setupName, UserExchangeManager::valid
	 */
	Q_PROPERTY(QQmlAccountManager* manager READ manager WRITE setManager NOTIFY managerChanged)
	/*! @brief Specifies if the object was correctly initialized
	 *
	 * @default{`false`}
	 *
	 * Only becomes true if the given setup name resolves to a valid datasync
	 * instance or if the given manager is valid
	 *
	 * @accessors{
	 *	@memberAc{valid}
	 *	@notifyAc{validChanged()}
	 *  @readonlyAc
	 * }
	 *
	 * @sa UserExchangeManager::setupName, UserExchangeManager::manager
	 */
	Q_PROPERTY(bool valid READ valid NOTIFY validChanged)

public:
	//! @private
	explicit QQmlUserExchangeManager(QObject *parent = nullptr);

	//! @private
	void classBegin() override;
	//! @private
	void componentComplete() override;

	//! @private
	quint16 DataExchangePort() const;

	//! @private
	QString setupName() const;
	//! @private
	QQmlAccountManager* manager() const;
	//! @private
	bool valid() const;

	//! @copydoc ::QtDataSync::UserExchangeManager::importFrom(const QtDataSync::UserInfo &, const std::function<void(bool,QString)> &, bool)
	Q_INVOKABLE void importFrom(const QtDataSync::UserInfo &userInfo,
								const QJSValue &completedFn,
								bool keepData = false);
	//! @copydoc ::QtDataSync::UserExchangeManager::importTrustedFrom(const QtDataSync::UserInfo &, const QString &, const std::function<void(bool,QString)> &, bool)
	Q_INVOKABLE void importTrustedFrom(const QtDataSync::UserInfo &userInfo,
									   const QString &password,
									   const QJSValue &completedFn,
									   bool keepData = false);

public Q_SLOTS:
	//! @private
	void setSetupName(QString setupName);
	//! @private
	void setManager(QQmlAccountManager* manager);

Q_SIGNALS:
	//! @notifyAcFn{AccountManager::setupName}
	void setupNameChanged(QString setupName);
	//! @notifyAcFn{AccountManager::manager}
	void managerChanged(QQmlAccountManager* manager);
	//! @notifyAcFn{AccountManager::valid}
	void validChanged(bool valid);

private:
	QString _setupName;
	QQmlAccountManager* _manager;
};

}

#endif // QQMLUSEREXCHANGEMANAGER_H
