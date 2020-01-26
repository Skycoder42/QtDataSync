#include <QtTest>
#include <QtDataSync/ICloudTransformer>
using namespace QtDataSync;

class CloudTransformerTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testTransform_data();
	void testTransform();

private:
	PlainCloudTransformer *_transformer = nullptr;

	QString serialize(const QVariant &data);
};

void CloudTransformerTest::initTestCase()
{
	_transformer = new PlainCloudTransformer{this};
}

void CloudTransformerTest::cleanupTestCase()
{
	_transformer->deleteLater();
}

void CloudTransformerTest::testTransform_data()
{
	QTest::addColumn<LocalData>("local");
	QTest::addColumn<CloudData>("cloud");

	const auto tStamp = QDateTime::currentDateTimeUtc();
	QTest::addRow("keys.simple") << LocalData {
		QStringLiteral("SimpleType"), QStringLiteral("simpleKey"),
		std::nullopt,
		tStamp,
		QDateTime{}
	} << CloudData {
		QStringLiteral("SimpleType"), QStringLiteral("_simpleKey"),
		std::nullopt,
		tStamp,
		QDateTime{}
	};

	QTest::addRow("keys.number") << LocalData {
		QStringLiteral("SimpleType"), QStringLiteral("42"),
		std::nullopt,
		tStamp,
		QDateTime{}
	} << CloudData {
		QStringLiteral("SimpleType"), QStringLiteral("_42"),
		std::nullopt,
		tStamp,
		QDateTime{}
	};

	QTest::addRow("keys.complex") << LocalData {
		QStringLiteral("QMap<QString, int>"),
		QString::fromWCharArray(L"kdsvj\"!\'§383942¬{¬¼½³¼²³¼²{{32423}}"),
		std::nullopt,
		tStamp,
		QDateTime{}
	} << CloudData {
		QStringLiteral("QMap%3CQString%2C%20int%3E"),
		QStringLiteral("_kdsvj%22%21%27%C2%A7383942%C2%AC%7B%C2%AC%C2%BC%C2%BD%C2%B3%C2%BC%C2%B2%C2%B3%C2%BC%C2%B2%7B%7B32423%7D%7D"),
		std::nullopt,
		tStamp,
		QDateTime{}
	};

	QTest::addRow("data.simple") << LocalData {
		QStringLiteral("SimpleType"), QStringLiteral("simpleKey"),
		QVariantHash {
			{QStringLiteral("key0"), true},
			{QStringLiteral("key1"), 42},
			{QStringLiteral("key2"), 1.3},
			{QStringLiteral("key3"), QVariant::fromValue(nullptr)},
			{QStringLiteral("key4"), QStringLiteral("test")},
			{QStringLiteral("key5"), QVariantList{1, 2, 3}},
			{QStringLiteral("key6"), QVariantHash {
				{QStringLiteral("a"), 1},
				{QStringLiteral("b"), 2},
				{QStringLiteral("c"), 3},
			}}
		},
		tStamp,
		QDateTime{}
	} << CloudData {
		QStringLiteral("SimpleType"), QStringLiteral("_simpleKey"),
		QJsonObject {
			{QStringLiteral("key0"), true},
			{QStringLiteral("key1"), 42},
			{QStringLiteral("key2"), 1.3},
			{QStringLiteral("key3"), QJsonValue::Null},
			{QStringLiteral("key4"), QStringLiteral("test")},
			{QStringLiteral("key5"), QJsonArray{1, 2, 3}},
			{QStringLiteral("key6"), QJsonObject {
				{QStringLiteral("a"), 1},
				{QStringLiteral("b"), 2},
				{QStringLiteral("c"), 3},
			}}
		},
		tStamp,
		QDateTime{}
	};

	QTest::addRow("data.bytearray") << LocalData {
		QStringLiteral("SimpleType"), QStringLiteral("simpleKey"),
		QVariantHash {
			{QStringLiteral("key"), QByteArray{"Hello World"}}
		},
		tStamp,
		QDateTime{}
	} << CloudData {
		QStringLiteral("SimpleType"), QStringLiteral("_simpleKey"),
		QJsonObject {
			{QStringLiteral("key"), QJsonArray{QStringLiteral("__qtds_b"), QStringLiteral("SGVsbG8gV29ybGQ=")}}
		},
		tStamp,
		QDateTime{}
	};

	QTest::addRow("data.url") << LocalData {
		QStringLiteral("SimpleType"), QStringLiteral("simpleKey"),
		QVariantHash {
			{QStringLiteral("key"), QUrl{QStringLiteral("https://example.com/test?b=4&a=2#um")}}
		},
		tStamp,
		QDateTime{}
	} << CloudData {
		QStringLiteral("SimpleType"), QStringLiteral("_simpleKey"),
		QJsonObject {
			{QStringLiteral("key"), QJsonArray{QStringLiteral("__qtds_u"), QStringLiteral("https://example.com/test?b=4&a=2#um")}}
		},
		tStamp,
		QDateTime{}
	};

	QTest::addRow("data.datetime") << LocalData {
		QStringLiteral("SimpleType"), QStringLiteral("simpleKey"),
		QVariantHash {
			{QStringLiteral("key"), QDateTime{{2010, 10, 5}, {14, 27, 7, 550}, Qt::UTC}}
		},
		tStamp,
		QDateTime{}
	} << CloudData {
		QStringLiteral("SimpleType"), QStringLiteral("_simpleKey"),
		QJsonObject {
			{QStringLiteral("key"), QJsonArray{QStringLiteral("__qtds_d"), QStringLiteral("2010-10-05T14:27:07.550Z")}}
		},
		tStamp,
		QDateTime{}
	};

	const auto uuid = QUuid::createUuid();
	QTest::addRow("data.uuid") << LocalData {
		QStringLiteral("SimpleType"), QStringLiteral("simpleKey"),
		QVariantHash {
			{QStringLiteral("key"), uuid}
		},
		tStamp,
		QDateTime{}
	} << CloudData {
		QStringLiteral("SimpleType"), QStringLiteral("_simpleKey"),
		QJsonObject {
			{QStringLiteral("key"), QJsonArray{QStringLiteral("__qtds_i"), uuid.toString(QUuid::WithoutBraces)}}
		},
		tStamp,
		QDateTime{}
	};

	QTest::addRow("data.integers") << LocalData {
		QStringLiteral("SimpleType"), QStringLiteral("simpleKey"),
		QVariantHash {
			{QStringLiteral("keyC"), std::numeric_limits<char>::max()},
			{QStringLiteral("keyI8"), std::numeric_limits<qint8>::min()},
			{QStringLiteral("keyU8"), std::numeric_limits<quint8>::max()},
			{QStringLiteral("keyI16"), std::numeric_limits<qint16>::min()},
			{QStringLiteral("keyU16"), std::numeric_limits<quint16>::max()},
			{QStringLiteral("keyI32"), std::numeric_limits<qint32>::min()},
			{QStringLiteral("keyU32"), std::numeric_limits<quint32>::max()},
			{QStringLiteral("keyI64"), std::numeric_limits<qint64>::min()},
			{QStringLiteral("keyU64"), std::numeric_limits<quint64>::max()},
		},
		tStamp,
		QDateTime{}
	} << CloudData {
		QStringLiteral("SimpleType"), QStringLiteral("_simpleKey"),
		QJsonObject {
			{QStringLiteral("keyC"), static_cast<int>(std::numeric_limits<char>::max())},
			{QStringLiteral("keyI8"), static_cast<int>(std::numeric_limits<qint8>::min())},
			{QStringLiteral("keyU8"), static_cast<int>(std::numeric_limits<quint8>::max())},
			{QStringLiteral("keyI16"), static_cast<int>(std::numeric_limits<qint16>::min())},
			{QStringLiteral("keyU16"), static_cast<int>(std::numeric_limits<quint16>::max())},
			{QStringLiteral("keyI32"), static_cast<int>(std::numeric_limits<qint32>::min())},
			{QStringLiteral("keyU32"), QJsonArray{QStringLiteral("__qtds_v"), QDataStream::Qt_DefaultCompiledVersion, serialize(std::numeric_limits<quint32>::max())}},
			{QStringLiteral("keyI64"), QJsonArray{QStringLiteral("__qtds_v"), QDataStream::Qt_DefaultCompiledVersion, serialize(std::numeric_limits<qint64>::min())}},
			{QStringLiteral("keyU64"), QJsonArray{QStringLiteral("__qtds_v"), QDataStream::Qt_DefaultCompiledVersion, serialize(std::numeric_limits<quint64>::max())}},
		},
		tStamp,
		QDateTime{}
	};

	const QPoint p{42, 24};
	QTest::addRow("data.point") << LocalData {
		QStringLiteral("SimpleType"), QStringLiteral("simpleKey"),
		QVariantHash {
			{QStringLiteral("key"), p},
		},
		tStamp,
		QDateTime{}
	} << CloudData {
		QStringLiteral("SimpleType"), QStringLiteral("_simpleKey"),
		QJsonObject {
			{QStringLiteral("key"), QJsonArray{QStringLiteral("__qtds_v"), QDataStream::Qt_DefaultCompiledVersion, serialize(p)}},
		},
		tStamp,
		QDateTime{}
	};
}

void CloudTransformerTest::testTransform()
{
	QFETCH(LocalData, local);
	QFETCH(CloudData, cloud);

	QSignalSpy transUpSpy{_transformer, &ICloudTransformer::transformUploadDone};
	QVERIFY(transUpSpy.isValid());
	QSignalSpy transDownSpy{_transformer, &ICloudTransformer::transformDownloadDone};
	QVERIFY(transDownSpy.isValid());
	QSignalSpy errorSpy{_transformer, &ICloudTransformer::transformError};
	QVERIFY(errorSpy.isValid());

	QCOMPARE(_transformer->escapeType(local.key().typeName), cloud.key().typeName);
	QCOMPARE(_transformer->unescapeType(cloud.key().typeName), local.key().typeName);

	QCOMPARE(_transformer->escapeKey(local.key()), cloud.key());
	QCOMPARE(_transformer->unescapeKey(cloud.key()), local.key());

	_transformer->transformUpload(local);
	QTRY_COMPARE(transUpSpy.size(), 1);
	QVERIFY(errorSpy.isEmpty());
	QCOMPARE(transUpSpy[0][0].value<CloudData>(), cloud);

	_transformer->transformDownload(cloud);
	QTRY_COMPARE(transDownSpy.size(), 1);
	QVERIFY(errorSpy.isEmpty());
	QCOMPARE(transDownSpy[0][0].value<LocalData>(), local);
}

QString CloudTransformerTest::serialize(const QVariant &data)
{
	QByteArray ba;
	QDataStream stream{&ba, QIODevice::WriteOnly};
	stream << data;
	return QString::fromUtf8(ba.toBase64());
}

QTEST_MAIN(CloudTransformerTest)

#include "tst_cloudtransformer.moc"
