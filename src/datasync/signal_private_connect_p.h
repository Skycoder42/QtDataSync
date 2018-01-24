#ifndef QTDATASYNC_SIGNAL_PRIVATE_CONNECT_P_H
#define QTDATASYNC_SIGNAL_PRIVATE_CONNECT_P_H

#include <QtCore/QObject>

#define extend(a, x, ...) [a](auto... args) { \
	(a->*(x))(args..., __VA_ARGS__); \
}

#define private_connect(a, x, b, y) QObject::connect(a, x, b, extend(b, y, QPrivateSignal{}))

#endif // QTDATASYNC_SIGNAL_PRIVATE_CONNECT_P_H
