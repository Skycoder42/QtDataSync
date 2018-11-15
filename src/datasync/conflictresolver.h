#ifndef QTDATASYNC_CONFLICTRESOLVER_H
#define QTDATASYNC_CONFLICTRESOLVER_H

#include <QtCore/qobject.h>
#include <QtCore/qscopedpointer.h>

#include <QtJsonSerializer/qjsonserializer.h>

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/defaults.h"
#include "QtDataSync/logger.h"

namespace QtDataSync {

class ConflictResolverPrivate;
//! Interface to implement a custom conflict handler for sync conflicts
class Q_DATASYNC_EXPORT ConflictResolver : public QObject
{
	Q_OBJECT

public:
	//! Default constructor
	explicit ConflictResolver(QObject *parent = nullptr);
	~ConflictResolver() override;

	//! @private
	void setDefaults(const Defaults &defaults);

	//! Returns the name of the setup this class operates on
	QString setupName() const;
	//! The resolvers name. Used for logging
	virtual QByteArray name() const;
	//! The method called to resolve conflicts between two datasets
	virtual QJsonObject resolveConflict(int typeId, const QJsonObject &data1, const QJsonObject &data2) const = 0;

protected:
	//! Access to the defaults of the current datasync instance
	Defaults defaults() const;
	//! Returns the logger that should be used to log merge incidents
	Logger *logger() const;
	//! Access to the resolvers settings section of the current datasync instance
	QSettings *settings() const;

private:
	QScopedPointer<ConflictResolverPrivate> d;
};

//! A generic variant of the ConflictResolver to handel dataobjects instead of json data
template <typename T1, typename... Args>
class GenericConflictResolver : public GenericConflictResolver<Args...>
{
public:
	//! Default constructor
	inline GenericConflictResolver(QObject *parent = nullptr) :
		GenericConflictResolver<Args...>(parent)
	{}

	//! @copydoc ConflictResolver::resolveConflict
	inline QJsonObject resolveConflict(int typeId, const QJsonObject &data1, const QJsonObject &data2) const override {
		if(typeId == qMetaTypeId<T1>()) {
			try {
				QObject scope;
				const QJsonSerializer *ser = this->defaults().serializer();
				auto d1 = ser->deserialize<T1>(data1, &scope);
				auto d2 = ser->deserialize<T1>(data2, &scope);
				return ser->serialize<T1>(resolveConflict(d1, d2, &scope));
			} catch(NoConflictResultException&) {
				return QJsonObject();
			}
		} else
			return GenericConflictResolver<Args...>::resolveConflict(typeId, data1, data2);
	}

protected:
	//! @private
	using NoConflictResultException = typename GenericConflictResolver<Args...>::NoConflictResultException;

	/*!
	 * @copybrief ConflictResolver::resolveConflict
	 *
	 * @param data1 The first dataset
	 * @param data2 The second dataset
	 * @param parent A temporary qobject to use as parent for created QObjects
	 * @returns The independent merge result of the two datasets
	 * @throws NoConflictResultException Thrown if resolving was not possible and the default JSON-based
	 * resolving should be used instead
	 * @throws QException In case data corruption was detected an exception can be thrown to abort the
	 * synchronization. This will put the engine in an (unrecoverable) error state.
	 *
	 * Must be implemented as the main method of the resolver. The method *must* always return something.
	 * If you find that you cannot decide how to merge, simply throw the `NoConflictResultException`.
	 * In that case, the library will ignore the result and proceed to merge as if no conflict resolver
	 * was present. It's basically this methods equivalent of returning an empty JSON object on the
	 * non generic variant of this method.
	 *
	 * @warning This method **must** be deterministic and independent of the parameter order. This means
	 * if you assume you have 2 objects, no matter at what point in time and in which order they are
	 * passed to this method, the returned object **must** always be the same. If thats not the case it
	 * can lead to infinite synchronisation loops.
	 *
	 * @sa GenericConflictResolver::resolveUnknownConflict, ConflictResolver::resolveConflict
	 */ //must be documented here because doxygen wont find it otherwise
	virtual T1 resolveConflict(T1 data1, T1 data2, QObject *parent) const = 0;
};

//! @copydoc QtDataSync::GenericConflictResolver
template <typename T1>
class GenericConflictResolver<T1> : public ConflictResolver
{
public:
	//! @copydoc GenericConflictResolver::GenericConflictResolver
	inline GenericConflictResolver(QObject *parent = nullptr) :
		ConflictResolver(parent)
	{}

	inline QJsonObject resolveConflict(int typeId, const QJsonObject &data1, const QJsonObject &data2) const override {
		if(typeId == qMetaTypeId<T1>()) {
			try {
				QObject scope;
				const QJsonSerializer *ser = this->defaults().serializer();
				auto d1 = ser->deserialize<T1>(data1, &scope);
				auto d2 = ser->deserialize<T1>(data2, &scope);
				return ser->serialize<T1>(resolveConflict(d1, d2, &scope));
			} catch(NoConflictResultException&) {
				return QJsonObject();
			}
		} else
			return resolveUnknownConflict(typeId, data1, data2);
	}

protected:
	//! An exception to be thrown from resolveConflict() if resolving was not possible and the default JSON-based resolving should be used instead
	class NoConflictResultException {};

	/*!
	 * @copybrief ConflictResolver::resolveConflict
	 *
	 * @param data1 The first dataset
	 * @param data2 The second dataset
	 * @param parent A temporary qobject to use as parent for created QObjects
	 * @returns The independent merge result of the two datasets
	 * @throws NoConflictResultException Thrown if resolving was not possible and the default JSON-based
	 * resolving should be used instead
	 * @throws QException In case data corruption was detected an exception can be thrown to abort the
	 * synchronization. This will put the engine in an (unrecoverable) error state.
	 *
	 * Must be implemented as the main method of the resolver. The method *must* always return something.
	 * If you find that you cannot decide how to merge, simply throw the `NoConflictResultException`.
	 * In that case, the library will ignore the result and proceed to merge as if no conflict resolver
	 * was present. It's basically this methods equivalent of returning an empty JSON object on the
	 * non generic variant of this method.
	 *
	 * @warning This method **must** be deterministic and independent of the parameter order. This means
	 * if you assume you have 2 objects, no matter at what point in time and in which order they are
	 * passed to this method, the returned object **must** always be the same. If thats not the case it
	 * can lead to infinite synchronisation loops.
	 *
	 * @sa GenericConflictResolver::resolveUnknownConflict, ConflictResolver::resolveConflict
	 */ //must be documented here because doxygen wont find it otherwise
	virtual T1 resolveConflict(T1 data1, T1 data2, QObject *parent) const = 0;

	/*!
	 * Method called for any type that was not handeled generically
	 *
	 * Behaves the same as ConflictResolver::resolveConflict. The default implementation will simply
	 * log the incident and return an empty json object to let the engine handle the merge.
	 *
	 * @sa ConflictResolver::resolveConflict
	 */
	virtual inline QJsonObject resolveUnknownConflict(int typeId, const QJsonObject &data1, const QJsonObject &data2) const {
		Q_UNUSED(data1)
		Q_UNUSED(data2)
		QT_DATASYNC_LOG_BASE.warning(logger()->loggingCategory()) << "Unsupported type in conflict resolver:"
																  << QMetaType::typeName(typeId);
		return QJsonObject();
	}
};

}

#endif // QTDATASYNC_CONFLICTRESOLVER_H
