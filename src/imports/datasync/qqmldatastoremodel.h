#ifndef QQMLDATASTOREMODEL_H
#define QQMLDATASTOREMODEL_H

#include <QtCore/QObject>

#include <QtQml/QQmlParserStatus>

#include <QtDataSync/datastoremodel.h>

#include "qqmldatastore.h"

#ifdef DOXYGEN_RUN
namespace de::skycoder42::QtDataSync {

/*! @brief The QML binding of ::QtDataSync::DataStoreModel
 *
 * @since 4.0
 *
 * @sa QtDataSync::DataStoreModel
 */
class DataStoreModel : public ::QtDataSync::DataStoreModel
#else
namespace QtDataSync {

class QQmlDataStoreModel : public DataStoreModel, public QQmlParserStatus
#endif
{
	Q_OBJECT
	Q_DISABLE_COPY(QQmlDataStoreModel)
	Q_INTERFACES(QQmlParserStatus)

	/*! @brief Holds the name of the setup this model operates on.
	 *
	 * @default{`QtDataSync::DefaultSetup`}
	 *
	 * Allows you to specify the name of the setup that this model should use
	 *
	 * @warning This property must only ever be set on construction of the object
	 * and must not be changed afterwards. Do not set both, this property and dataStore
	 *
	 * @accessors{
	 *	@memberAc{setupName}
	 *	@notifyAc{setupNameChanged()}
	 * }
	 *
	 * @sa ::QtDataSync::DataStoreModel::setupName, DataStoreModel::dataStore,
	 * DataStoreModel::valid
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
	 * @sa DataStoreModel::setupName, DataStoreModel::valid
	 */
	Q_PROPERTY(QQmlDataStore* dataStore READ dataStore WRITE setDataStore NOTIFY dataStoreChanged)
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
	 * @sa DataStoreModel::setupName, DataStoreModel::dataStore
	 */
	Q_PROPERTY(bool valid READ valid NOTIFY validChanged)
	/*! @brief Holds the name of the type this model shall present
	 *
	 * @default{<i>empty</i>}
	 *
	 * This property internally calls ::QtDataSync::DataStoreModel::setTypeId
	 * to set or switch the type that is used by the model to load and present data.
	 *
	 * @accessors{
	 *	@memberAc{typeName}
	 *	@notifyAc{typeNameChanged()}
	 * }
	 *
	 * @sa ::QtDataSync::DataStoreModel::setTypeId
	 */
	Q_PROPERTY(QString typeName READ typeName WRITE setTypeName NOTIFY typeNameChanged)

public:
	//! @private
	explicit QQmlDataStoreModel(QObject *parent = nullptr);

	//! @private
	void classBegin() override;
	//! @private
	void componentComplete() override;

	//! @private
	QString setupName() const;
	//! @private
	QQmlDataStore* dataStore() const;
	//! @private
	bool valid() const;
	//! @private
	QString typeName() const;

public Q_SLOTS:
	//! @private
	void setSetupName(QString setupName);
	//! @private
	void setDataStore(QQmlDataStore* dataStore);
	//! @private
	void setTypeName(const QString &typeName);

Q_SIGNALS:
	//! @notifyAcFn{AccountManager::setupName}
	void setupNameChanged(const QString &setupName);
	//! @notifyAcFn{AccountManager::dataStore}
	void dataStoreChanged(QQmlDataStore* dataStore);
	//! @notifyAcFn{AccountManager::valid}
	void validChanged(bool valid);
	//! @notifyAcFn{AccountManager::typeName}
	void typeNameChanged(const QString &typeName);

private:
	QString _setupName;
	QQmlDataStore* _dataStore;
};

}

#endif // QQMLDATASTOREMODEL_H
