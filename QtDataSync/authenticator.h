#ifndef AUTHENTICATOR_H
#define AUTHENTICATOR_H

#include "qtdatasync_global.h"
#include "remoteconnector.h"
#include <QObject>
#include <QPointer>
#include <type_traits>

namespace QtDataSync {

class QTDATASYNCSHARED_EXPORT Authenticator : public QObject
{
	Q_OBJECT

public:
	explicit Authenticator(RemoteConnector *connector, QObject *parent = nullptr);

protected:
	template<typename T>
	T *connector() const;

private:
	QPointer<RemoteConnector> _connector;
};

template<typename T>
T *Authenticator::connector() const
{
	static_assert(std::is_base_of<RemoteConnector, T>::value, "T must inherit QtDataSync::RemoteConnector!");
	return static_cast<T*>(_connector.data());
}

}

#endif // AUTHENTICATOR_H
