#ifndef QTDATASYNC_HELPERTYPES_H
#define QTDATASYNC_HELPERTYPES_H

#include <type_traits>

#include <QtCore/qobject.h>

#include "QtDataSync/qtdatasync_global.h"

namespace QtDataSync {
namespace __helpertypes {

template <class T, class Enable = void>
struct is_gadget : public std::false_type {};

template <class T>
struct is_gadget<T, typename T::QtGadgetHelper> : public std::true_type {};

template <class T>
struct is_object : public std::false_type {};

template <class T>
struct is_object<T*> : public std::is_base_of<QObject, T> {};

template <typename T>
struct is_storable : public is_gadget<T> {};

template <typename T>
struct is_storable<T*> : public std::is_base_of<QObject, T> {};

}
}

#define QTDATASYNC_STORE_ASSERT(T) static_assert(__helpertypes::is_storable<T>::value, "Only Q_GADGETS or pointers to QObject extending classes can be stored")

#endif // QTDATASYNC_HELPERTYPES_H
