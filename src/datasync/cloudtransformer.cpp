#include "cloudtransformer.h"
#include "cloudtransformer_p.h"
#include <QtCore/QUrl>
#include <QtCore/QUuid>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonValue>
#include <QtCore/QDataStream>
using namespace QtDataSync;

Q_LOGGING_CATEGORY(QtDataSync::logTrans, "qt.datasync.ICloudTransformer")

QString ICloudTransformer::escapeType(const QString &name)
{
	return QString::fromUtf8(QUrl::toPercentEncoding(name));
}

QString ICloudTransformer::unescapeType(const QString &name)
{
	return QUrl::fromPercentEncoding(name.toUtf8());
}

DatasetId ICloudTransformer::escapeKey(const DatasetId &key)
{
	return {
		escapeType(key.tableName),
		QLatin1Char('_') + QString::fromUtf8(QUrl::toPercentEncoding(key.key)) // prepend _ to not have arrays
	};
}

DatasetId ICloudTransformer::unescapeKey(const DatasetId &key)
{
	Q_ASSERT_X(key.key.startsWith(QLatin1Char('_')), Q_FUNC_INFO, "Invalid object key. id should start with an _");
	return {
		unescapeType(key.tableName),
		QUrl::fromPercentEncoding(key.key.mid(1).toUtf8())  // remove _ at beginning
	};
}

ICloudTransformer::ICloudTransformer(QObject *parent) :
	  QObject{parent}
{}

ICloudTransformer::ICloudTransformer(QObjectPrivate &dd, QObject *parent) :
	  QObject{dd, parent}
{}



void ISynchronousCloudTransformer::transformUpload(const LocalData &data)
{
	try {
		if (data.data())
			Q_EMIT transformUploadDone({escapeKey(data.key()), transformUploadSync(*data.data()), data});
		else
			Q_EMIT transformUploadDone({escapeKey(data.key()), std::nullopt, data});
	} catch (std::exception &e) {
		qCWarning(logTrans) << "Failed to transform upload for key" << data.key()
							<< "with error:" << e.what();
		Q_EMIT transformError(data.key());
	}
}

void ISynchronousCloudTransformer::transformDownload(const CloudData &data)
{
	const auto unEscKey = unescapeKey(data.key());
	try {
		if (data.data())
			Q_EMIT transformDownloadDone({unEscKey, transformDownloadSync(*data.data()), data});
		else
			Q_EMIT transformDownloadDone({unEscKey, std::nullopt, data});
	} catch (std::exception &e) {
		qCWarning(logTrans) << "Failed to transform download for key" << unEscKey
							<< "with error:" << e.what();
		Q_EMIT transformError(unEscKey);
	}
}

ISynchronousCloudTransformer::ISynchronousCloudTransformer(QObject *parent) :
	  ICloudTransformer{parent}
{}

ISynchronousCloudTransformer::ISynchronousCloudTransformer(QObjectPrivate &dd, QObject *parent) :
	  ICloudTransformer{dd, parent}
{}



const QString PlainCloudTransformerPrivate::MarkerByteArray = QStringLiteral("__qtds_b");
const QString PlainCloudTransformerPrivate::MarkerUrl = QStringLiteral("__qtds_u");
const QString PlainCloudTransformerPrivate::MarkerDateTime = QStringLiteral("__qtds_d");
const QString PlainCloudTransformerPrivate::MarkerUuid = QStringLiteral("__qtds_i");
const QString PlainCloudTransformerPrivate::MarkerVariant = QStringLiteral("__qtds_v");

PlainCloudTransformer::PlainCloudTransformer(QObject *parent) :
	ISynchronousCloudTransformer{*new PlainCloudTransformerPrivate{}, parent}
{}

QJsonObject PlainCloudTransformer::transformUploadSync(const QVariantHash &data) const
{
	Q_D(const PlainCloudTransformer);
	return d->serialize(data).toObject();

}

QVariantHash PlainCloudTransformer::transformDownloadSync(const QJsonObject &data) const
{
	Q_D(const PlainCloudTransformer);
	return d->deserialize(data).toHash();
}

QJsonValue PlainCloudTransformerPrivate::serialize(const QVariant &data) const
{
	auto marker = MarkerVariant;
	switch (data.userType()) {
	case QMetaType::QStringList:
	case QMetaType::QVariantList: {
		QJsonArray arr;
		for (const auto val : data.toList())
			arr.append(serialize(val));
		return arr;
	}
	case QMetaType::QVariantMap:
	case QMetaType::QVariantHash: {
		QJsonObject obj;
		const auto map = data.toHash();
		for (auto it = map.begin(), end = map.end(); it != end; ++it)
			obj.insert(it.key(), serialize(*it));
		return obj;
	}
	case QMetaType::Nullptr:
	case QMetaType::UnknownType:
		return QJsonValue::Null;
	case QMetaType::Bool:
		return data.toBool();
	case QMetaType::QString:
		return data.toString();
	case QMetaType::Double:
	case QMetaType::Float:
		return data.toDouble();
	case QMetaType::QJsonValue:
		return data.toJsonValue();
	case QMetaType::QJsonObject:
		return data.toJsonObject();
	case QMetaType::QJsonArray:
		return data.toJsonArray();
	case QMetaType::QByteArray:
		return QJsonArray {
			MarkerByteArray,
			QString::fromUtf8(data.toByteArray().toBase64(QByteArray::Base64Encoding | QByteArray::KeepTrailingEquals))
		};
	case QMetaType::QUrl:
		return QJsonArray {
			MarkerUrl,
			data.toUrl().toString(QUrl::FullyEncoded)
		};
	case QMetaType::QDateTime:
		return QJsonArray {
			MarkerDateTime,
			data.toDateTime().toUTC().toString(Qt::ISODateWithMs)
		};
	case QMetaType::QUuid:
		return QJsonArray {
			MarkerUuid,
			data.toUuid().toString(QUuid::WithoutBraces)
		};
	case QMetaType::Char:
	case QMetaType::SChar:
	case QMetaType::UChar:
	case QMetaType::Short:
	case QMetaType::UShort:
	case QMetaType::Int:
	case QMetaType::Long:
	// --- The following types are exlcuded and stored as binary, because they exceed the qt json integer limits
	//case QMetaType::UInt:
	//case QMetaType::ULong:
	//case QMetaType::LongLong:
	//case QMetaType::ULongLong:
		if constexpr(sizeof(long) > sizeof (int)) {
			if (data.userType() == QMetaType::Long)
				Q_FALLTHROUGH();
			else
				return data.toInt();
		} else
			return data.toInt();
	default: {
		QByteArray binData;
		QDataStream stream{&binData, QIODevice::WriteOnly};
		stream << data;
		return QJsonArray {
			MarkerVariant,
			stream.version(),
			QString::fromUtf8(binData.toBase64(QByteArray::Base64Encoding | QByteArray::KeepTrailingEquals))
		};
	}
	}
}

QVariant PlainCloudTransformerPrivate::deserialize(const QJsonValue &json) const
{
	switch (json.type()) {
	case QJsonValue::Undefined:
		return {};
	case QJsonValue::Null:
		return QVariant::fromValue(nullptr);
	case QJsonValue::Bool:
		return json.toBool();
	case QJsonValue::Double:
		return json.toDouble();
	case QJsonValue::String:
		return json.toString();
	case QJsonValue::Object: {
		QVariantHash map;
		const auto obj = json.toObject();
		for (auto it = obj.begin(), end = obj.end(); it != end; ++it)
			map.insert(it.key(), deserialize(*it));
		return map;
	}
	case QJsonValue::Array: {
		const auto arr = json.toArray();
		if (arr.isEmpty())
			return QVariantList{};
		else {
			const auto marker = arr[0].toString();
			if (marker == MarkerByteArray && arr.size() == 2)
				return QByteArray::fromBase64(arr[1].toString().toUtf8(), QByteArray::Base64Encoding);
			else if (marker == MarkerUrl && arr.size() == 2)
				return QUrl{arr[1].toString()};
			else if (marker == MarkerDateTime && arr.size() == 2)
				return QDateTime::fromString(arr[1].toString(), Qt::ISODateWithMs);
			else if (marker == MarkerUuid && arr.size() == 2)
				return QUuid::fromString(arr[1].toString());
			else if (marker == MarkerVariant && arr.size() == 3) {
				const auto data = QByteArray::fromBase64(arr[2].toString().toUtf8(), QByteArray::Base64Encoding);
				QDataStream stream{data};
				stream.setVersion(arr[1].toInt());
				QVariant res;
				stream.startTransaction();
				stream >> res;
				if (stream.commitTransaction())
					return res;
				else
					throw VariantException{QStringLiteral("Failed to parse received variant with status: %1").arg(stream.status())};
			} else {
				QVariantList res;
				res.reserve(arr.size());
				for (const auto &val : arr)
					res.append(deserialize(val));
				return res;
			}
		}
	}
	default:
		Q_UNREACHABLE();
	}
}



VariantException::VariantException(QString error) :
	_error{std::move(error)}
{}

QString VariantException::qWhat() const
{
	return _error;
}

void VariantException::raise() const
{
	throw *this;
}

ExceptionBase *VariantException::clone() const
{
	return new VariantException{*this};
}
