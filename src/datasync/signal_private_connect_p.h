#ifndef QTDATASYNC_SIGNAL_PRIVATE_CONNECT_P_H
#define QTDATASYNC_SIGNAL_PRIVATE_CONNECT_P_H

#include <functional>

namespace QtDataSync {
namespace private_connect {

template<typename... Args>
class PS
{
public:
	inline std::function<void(Args...)> fix(const std::function<void(Args...)> &fn) {
		return fn;
	}
};

template <typename TClass, typename... Args>
PS<Args...> extractParams(void(TClass::*ptr)(Args...)) {
	Q_UNUSED(ptr);
	return PS<Args...>{};
}

template <typename... Args>
inline const std::function<void(Args...)> &fix(const std::function<void(Args...)> &fn) {
	return fn;
}

}
}

#define ps(x, y) \
	QtDataSync::private_connect::extractParams(x).fix([this](auto... args) { \
		(this->*(y))(args..., QPrivateSignal{}); \
	})
#define private_connect(a, x, b, y) \
	QObject::connect(a, x, b, ps(x, y))

#endif // QTDATASYNC_SIGNAL_PRIVATE_CONNECT_P_H
