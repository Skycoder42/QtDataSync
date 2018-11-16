#ifndef QQMLDATASTORE_H
#define QQMLDATASTORE_H

#include <QtCore/QObject>

#include <QtQml/QQmlParserStatus>

#include <QtDataSync/datastore.h>

#ifdef DOXYGEN_RUN
namespace de::skycoder42::QtDataSync {

/*! @brief The QML binding of ::QtDataSync::DataStore
 *
 * @since 4.0
 *
 * @sa QtDataSync::DataStore
 */
class DataStore : public ::QtDataSync::DataStore
#else
namespace QtDataSync {

class QQmlDataStore : public DataStore, public QQmlParserStatus
#endif
{
	Q_OBJECT
	Q_DISABLE_COPY(QQmlDataStore)
	Q_INTERFACES(QQmlParserStatus)

	/*! @brief Holds the name of the setup this store operates on.
	 *
	 * @default{`QtDataSync::DefaultSetup`}
	 *
	 * Allows you to specify the name of the setup that this store should use
	 *
	 * @warning This property must only ever be set on construction of the object
	 * and must not be changed afterwards
	 *
	 * @accessors{
	 *	@memberAc{setupName}
	 *	@notifyAc{setupNameChanged()}
	 * }
	 *
	 * @sa ::QtDataSync::DataStore::setupName, DataStore::valid
	 */
	Q_PROPERTY(QString setupName READ setupName WRITE setSetupName NOTIFY setupNameChanged)
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
	 * @sa DataStore::setupName
	 */
	Q_PROPERTY(bool valid READ valid NOTIFY validChanged)

public:
	//! @private
	explicit QQmlDataStore(QObject *parent = nullptr);

	//! @private
	void classBegin() override;
	//! @private
	void componentComplete() override;

	//! @private
	QString setupName() const;
	//! @private
	bool valid() const;

	/*! @brief @copybrief ::QtDataSync::DataStore::count() const
	 *
	 * @param typeName The QMetaType type name of the type
	 * @returns The number of datasets of the given type stored
	 *
	 * @sa ::QtDataSync::DataStore::count(int) const
	 */
	Q_INVOKABLE qint64 count(const QString &typeName) const;
	/*! @brief @copybrief ::QtDataSync::DataStore::keys() const
	 *
	 * @param typeName The QMetaType type name of the type
	 * @returns A list of all keys stored for the given type
	 *
	 * @sa ::QtDataSync::DataStore::keys(int) const
	 */
	Q_INVOKABLE QStringList keys(const QString &typeName) const;
	/*! @brief @copybrief ::QtDataSync::DataStore::loadAll() const
	 *
	 * @param typeName The QMetaType type name of the type
	 * @returns A list of all datasets stored for the given type
	 *
	 * @sa ::QtDataSync::DataStore::loadAll(int) const
	 */
	Q_INVOKABLE QVariantList loadAll(const QString &typeName) const;
	/*! @brief @copybrief ::QtDataSync::DataStore::contains(const QString &) const
	 *
	 * @param typeName The QMetaType type name of the type
	 * @param key The key of the dataset to be checked for existance
	 * @returns true if the dataset exists, false if not
	 *
	 * @sa ::QtDataSync::DataStore::contains(int, const QString &) const
	 */
	Q_INVOKABLE QT_DATASYNC_REVISION_2 bool contains(const QString &typeName, const QString &key) const;
	/*! @brief @copybrief ::QtDataSync::DataStore::load(const QString &) const
	 *
	 * @param typeName The QMetaType type name of the type
	 * @param key The key of the dataset to be loaded
	 * @returns The dataset that was found for the given type and key
	 *
	 * @sa ::QtDataSync::DataStore::load(int, const QString &) const
	 */
	Q_INVOKABLE QVariant load(const QString &typeName, const QString &key) const;
	/*! @brief @copybrief ::QtDataSync::DataStore::save(const T &)
	 *
	 * @param typeName The QMetaType type name of the type
	 * @param value The dataset to be stored
	 *
	 * @sa ::QtDataSync::DataStore::save(int, QVariant)
	 */
	Q_INVOKABLE void save(const QString &typeName, const QVariant &value);
	/*! @brief @copybrief ::QtDataSync::DataStore::remove(const QString &)
	 *
	 * @param typeName The QMetaType type name of the type
	 * @param key The key of the dataset to be removed
	 * @returns `true` in case the dataset was removed, `false` if it did not exist
	 *
	 * @sa ::QtDataSync::DataStore::remove(int, const QString &)
	 */
	Q_INVOKABLE bool remove(const QString &typeName, const QString &key);
	/*! @brief @copybrief ::QtDataSync::DataStore::search(const QString &, DataStore::SearchMode) const
	 *
	 * @param typeName The QMetaType type name of the type
	 * @param query A search query to be used to find fitting datasets. Format depends on mode
	 * @param mode Specifies how to interpret the search `query` See DataStore::SearchMode documentation
	 * @returns A list with all datasets that keys matched the search query for the given type
	 *
	 * @sa ::QtDataSync::DataStore::search(int, const QString &, DataStore::SearchMode) const
	 */
	Q_INVOKABLE QVariantList search(const QString &typeName, const QString &query, DataStore::SearchMode mode = DataStore::RegexpMode) const;
	/*! @brief @copybrief ::QtDataSync::DataStore::clear()
	 *
	 * @param typeName The QMetaType type name of the type
	 *
	 * @sa ::QtDataSync::DataStore::clear(int)
	 */
	Q_INVOKABLE void clear(const QString &typeName);

	/*! @brief Returns the name of the type identified by the given type id
	 *
	 * @param typeId The QMetaType type id of the type
	 * @returns The type name of the type
	 */
	Q_INVOKABLE QString typeName(int typeId) const;

public Q_SLOTS:
	//! @private
	void setSetupName(QString setupName);

Q_SIGNALS:
	//! @notifyAcFn{AccountManager::setupName}
	void setupNameChanged(const QString &setupName);
	//! @notifyAcFn{AccountManager::valid}
	void validChanged(bool valid);

private:
	QString _setupName;
	bool _valid;
};

}

#endif // QQMLDATASTORE_H
