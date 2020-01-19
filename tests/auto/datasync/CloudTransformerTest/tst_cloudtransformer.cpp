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
			{QStringLiteral("key4"), QVariant::fromValue(nullptr)},
			{QStringLiteral("key5"), QStringLiteral("test")},
		},
		tStamp,
		QDateTime{}
	} << CloudData {
		QStringLiteral("SimpleType"), QStringLiteral("_simpleKey"),
		QJsonObject {
			{QStringLiteral("key0"), true},
			{QStringLiteral("key1"), 42},
			{QStringLiteral("key2"), 1.3},
			{QStringLiteral("key4"), QJsonValue::Null},
			{QStringLiteral("key5"), QStringLiteral("test")},
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
	QVERIFY2(errorSpy.isEmpty(), qUtf8Printable(errorSpy.value(0).value(1).toString()));
	QCOMPARE(transUpSpy[0][0].value<CloudData>(), cloud);

	_transformer->transformDownload(cloud);
	QTRY_COMPARE(transDownSpy.size(), 1);
	QVERIFY2(errorSpy.isEmpty(), qUtf8Printable(errorSpy.value(0).value(1).toString()));
	QCOMPARE(transDownSpy[0][0].value<LocalData>(), local);
}

QTEST_MAIN(CloudTransformerTest)

#include "tst_cloudtransformer.moc"
