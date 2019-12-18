#ifndef QTDATASYNC_FIREBASE_FIRESTORE_DOCUMENTFIELDS_H
#define QTDATASYNC_FIREBASE_FIRESTORE_DOCUMENTFIELDS_H

#include "qtdatasync_global.h"

#include <QtCore/QVariantMap>

#include <QtJsonSerializer/TypeConverter>

namespace QtDataSync::firebase::firestore {

class DocumentFields : public QVariantMap
{
public:
	DocumentFields() = default;
	DocumentFields(std::initializer_list<std::pair<QString, QVariant>> args);
	DocumentFields(const QVariantMap &other);
	DocumentFields(QVariantMap &&other) noexcept;
	DocumentFields(const DocumentFields &other) = default;
	DocumentFields(DocumentFields &&other) noexcept = default;

	DocumentFields &operator=(const QVariantMap &other);
	DocumentFields &operator=(QVariantMap &&other) noexcept;
	DocumentFields &operator=(const DocumentFields &other) = default;
	DocumentFields &operator=(DocumentFields &&other) noexcept = default;

	bool operator==(const DocumentFields &other) const;
	bool operator!=(const DocumentFields &other) const;
};

class DocumentFieldsTypeConverter : public QtJsonSerializer::TypeConverter
{
public:
	QT_JSONSERIALIZER_TYPECONVERTER_NAME(DocumentFieldsTypeConverter)
	bool canConvert(int metaTypeId) const override;
	QList<QCborValue::Type> allowedCborTypes(int metaTypeId, QCborTag tag) const override;
	QCborValue serialize(int propertyType, const QVariant &value) const override;
	QVariant deserializeCbor(int propertyType, const QCborValue &value, QObject *parent) const override;

private:
	QCborValue serializeValue(const QVariant &value) const;
	QVariant deserializeValue(const QCborValue &value) const;
};

}

Q_DECLARE_METATYPE(QtDataSync::firebase::firestore::DocumentFields)

#endif // QTDATASYNC_FIREBASE_FIRESTORE_DOCUMENTFIELDS_H
