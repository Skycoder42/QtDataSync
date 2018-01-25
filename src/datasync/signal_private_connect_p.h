#ifndef QTDATASYNC_SIGNAL_PRIVATE_CONNECT_P_H
#define QTDATASYNC_SIGNAL_PRIVATE_CONNECT_P_H

#include <QtCore/QObject>

#define EXTEND(a, x, ...) [a](auto... args) { \
	(a->*(x))(args..., __VA_ARGS__); \
}

#define PSIG(x) EXTEND(this, x, QPrivateSignal{})

#endif // QTDATASYNC_SIGNAL_PRIVATE_CONNECT_P_H
