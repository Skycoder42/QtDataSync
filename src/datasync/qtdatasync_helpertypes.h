#ifndef QTDATASYNC_HELPERTYPES_H
#define QTDATASYNC_HELPERTYPES_H

#include <QtCore/qobject.h>

#include <type_traits>

#include "QtDataSync/qtdatasync_global.h"

namespace QtDataSync {
namespace __helpertypes {

//! helper to get the gadget information
template <class T, class Enable = void>
struct is_gadget : public std::false_type {};

//! @copydoc _qjsonserializer_helpertypes::gadget_helper
template <class T>
struct is_gadget<T, typename T::QtGadgetHelper> : public std::true_type {};

//! helper to get the gadget information
template <class T>
struct is_object : public std::false_type {};

//! @copydoc _qjsonserializer_helpertypes::gadget_helper
template <class T>
struct is_object<T*> : public std::is_base_of<QObject, T> {};

//NOTE c++17 disjunction
//! test if a type can be stored
template <typename T>
struct is_storable : public is_gadget<T> {};

//! test if a type can be stored
template <typename T>
struct is_storable<T*> : public std::is_base_of<QObject, T> {};

}
}

#define QTDATASYNC_STORE_ASSERT(T) static_assert(__helpertypes::is_storable<T>::value, "Only Q_GADGETS or pointers to QObject extending classes can be stored")

#endif // QTDATASYNC_HELPERTYPES_H
