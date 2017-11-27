#ifndef QTDATASYNC_HELPERTYPES_H
#define QTDATASYNC_HELPERTYPES_H

#include <QtCore/qobject.h>

#include <type_traits>

#include "QtDataSync/qtdatasync_global.h"

namespace QtDataSync {
namespace __helpertypes {

//! helper to get the gadget information
template <class T, class Enable = void>
struct gadget_helper : public std::false_type {};

//! @copydoc _qjsonserializer_helpertypes::gadget_helper
template <class T>
struct gadget_helper<T, typename T::QtGadgetHelper> : public std::true_type {};

//! test if a type can be stored
template <typename T>
struct is_storable : public gadget_helper<T> {};

//! test if a type can be stored
template <typename T>
struct is_storable<T*> : public std::is_base_of<QObject, T> {};

//! test if a type can be stored
template <typename T>
struct is_storable_obj : public std::false_type {};

//! test if a type can be stored
template <typename T>
struct is_storable_obj<T*> : public std::is_base_of<QObject, T> {};

}
}

#define QTDATASYNC_STORE_ASSERT(T) static_assert(__helpertypes::is_storable<T>::value, "Only Q_GADGETS or pointers to QObject extending classes can be stored")

#endif // QTDATASYNC_HELPERTYPES_H
