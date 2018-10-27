#ifndef QTDATASYNC_SIGNAL_PRIVATE_CONNECT_P_H
#define QTDATASYNC_SIGNAL_PRIVATE_CONNECT_P_H

#include <QtCore/QObject>

#define EXTEND(a, x, ...) [a](auto... args) { \
	(a->*(x))(args..., __VA_ARGS__); \
}

#define EXTEND_NULL(a, x, ...) [a]() { \
	(a->*(x))(__VA_ARGS__); \
}

#define PSIG(x) EXTEND(this, x, QPrivateSignal{})

#define PSIG_NULL(x) EXTEND_NULL(this, x, QPrivateSignal{})

#if __cplusplus >= 201402L
#define LAMBDA_MV(x) x = std::move(x)
#else
#define LAMBDA_MV(x) x
#endif

#endif // QTDATASYNC_SIGNAL_PRIVATE_CONNECT_P_H
