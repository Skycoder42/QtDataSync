#ifndef QTDATASYNC_SYNCHELPER_P_H
#define QTDATASYNC_SYNCHELPER_P_H

#include <tuple>

#include <QtCore/QJsonObject>

#include "qtdatasync_global.h"
#include "objectkey.h"

namespace QtDataSync {

namespace SyncHelper {

//exports are needed for tests
Q_DATASYNC_EXPORT QByteArray jsonHash(const QJsonObject &object);

Q_DATASYNC_EXPORT QByteArray combine(const ObjectKey &key, quint64 version, const QJsonObject &data);
Q_DATASYNC_EXPORT QByteArray combine(const ObjectKey &key, quint64 version);
Q_DATASYNC_EXPORT std::tuple<bool, ObjectKey, quint64, QJsonObject> extract(const QByteArray &data); // (deleted, key, version, data)

}

}

#endif // QTDATASYNC_SYNCHELPER_P_H
