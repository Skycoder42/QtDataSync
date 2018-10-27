#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <testlib.h>
#include <testobject.h>
#include <QtDataSync/private/localstore_p.h>
#include <QtDataSync/private/defaults_p.h>
using namespace QtDataSync;

class TestEventCursor : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testPrepareData();
	void testCursorCreation();
	void testAdvanceAny();
	void testAdvanceSkip();
	void testScanLog();
	void testClearLog();
	void testReset();

private:
	QDateTime _firstStamp;
	QDateTime _lastStamp;
};

void TestEventCursor::initTestCase()
{
	try {
		TestLib::init();
		Setup setup;
		TestLib::setup(setup);
		setup.setEventLoggingMode(Setup::EventMode::Enabled)
				.create();
		QThread::sleep(1);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestEventCursor::cleanupTestCase()
{
	Setup::removeSetup(DefaultSetup, true);
}

void TestEventCursor::testPrepareData()
{
	try {
		QVERIFY(EventCursor::isEventLogActive());

		DataTypeStore<TestData, int> store;
		_firstStamp = QDateTime::currentDateTime();
		store.save(TestLib::generateData(2));
		store.save(TestLib::generateData(4));
		store.save(TestLib::generateData(6));
		store.save(TestData{6, QStringLiteral("60")});
		store.remove(4);
		_lastStamp = QDateTime::currentDateTime();

		QCOMPARE(store.count(), 2);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestEventCursor::testCursorCreation()
{
	try {
		auto cursor = EventCursor::first(this);
		QVERIFY(cursor);
		QVERIFY(cursor->isValid());
		QCOMPARE(cursor->index(), 1);
		QCOMPARE(cursor->key(), TestLib::generateKey(2));
		QCOMPARE(cursor->wasRemoved(), false);
		QVERIFY(cursor->timestamp() >= _firstStamp);
		QVERIFY(cursor->timestamp() <= _lastStamp);
		delete cursor;

		cursor = EventCursor::last(this);
		QVERIFY(cursor);
		QVERIFY(cursor->isValid());
		QCOMPARE(cursor->index(), 5);
		QCOMPARE(cursor->key(), TestLib::generateKey(4));
		QCOMPARE(cursor->wasRemoved(), true);
		QVERIFY(cursor->timestamp() >= _firstStamp);
		QVERIFY(cursor->timestamp() <= _lastStamp);
		delete cursor;

		cursor = EventCursor::create(3, this);
		QVERIFY(cursor);
		QVERIFY(cursor->isValid());
		QCOMPARE(cursor->index(), 3);
		QCOMPARE(cursor->key(), TestLib::generateKey(6));
		QCOMPARE(cursor->wasRemoved(), false);
		QVERIFY(cursor->timestamp() >= _firstStamp);
		QVERIFY(cursor->timestamp() <= _lastStamp);
		auto data = cursor->save();
		delete cursor;

		QVERIFY(!data.isEmpty());
		cursor = EventCursor::load(data, this);
		QVERIFY(cursor);
		QVERIFY(cursor->isValid());
		QCOMPARE(cursor->index(), 3);
		QCOMPARE(cursor->key(), TestLib::generateKey(6));
		QCOMPARE(cursor->wasRemoved(), false);
		QVERIFY(cursor->timestamp() >= _firstStamp);
		QVERIFY(cursor->timestamp() <= _lastStamp);
		delete cursor;
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestEventCursor::testAdvanceAny()
{
	try {
		auto cursor = EventCursor::first(this);
		QVERIFY(cursor);
		QVERIFY(cursor->isValid());
		QVERIFY(cursor->skipObsolete());
		cursor->setSkipObsolete(false);
		QVERIFY(!cursor->skipObsolete());

		QList<std::tuple<quint64, ObjectKey, bool>> dataList {
			std::make_tuple(1, TestLib::generateKey(2), false),
			std::make_tuple(2, TestLib::generateKey(4), false),
			std::make_tuple(3, TestLib::generateKey(6), false),
			std::make_tuple(4, TestLib::generateKey(6), false),
			std::make_tuple(5, TestLib::generateKey(4), true)
		};

		auto hasNext = false;
		do {
			QVERIFY(!dataList.isEmpty());
			quint64 index;
			ObjectKey key;
			bool wasRemoved;
			std::tie(index, key, wasRemoved) = dataList.takeFirst();

			QCOMPARE(cursor->index(), index);
			QCOMPARE(cursor->key(), key);
			QCOMPARE(cursor->wasRemoved(), wasRemoved);
			QVERIFY(cursor->timestamp() >= _firstStamp);
			QVERIFY(cursor->timestamp() <= _lastStamp);

			hasNext = cursor->hasNext();
			QCOMPARE(cursor->next(), hasNext);
		} while(hasNext);
		QVERIFY(dataList.isEmpty());

		delete cursor;
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestEventCursor::testAdvanceSkip()
{
	try {
		auto cursor = EventCursor::first(this);
		QVERIFY(cursor);
		QVERIFY(cursor->isValid());
		QVERIFY(cursor->skipObsolete());

		QList<std::tuple<quint64, ObjectKey, bool>> dataList {
			std::make_tuple(1, TestLib::generateKey(2), false),
			std::make_tuple(4, TestLib::generateKey(6), false),
			std::make_tuple(5, TestLib::generateKey(4), true)
		};

		auto hasNext = false;
		do {
			QVERIFY(!dataList.isEmpty());
			quint64 index;
			ObjectKey key;
			bool wasRemoved;
			std::tie(index, key, wasRemoved) = dataList.takeFirst();
			//TODO c++17: auto[index, key, wasRemoved] = dataList.takeFirst();

			QCOMPARE(cursor->index(), index);
			QCOMPARE(cursor->key(), key);
			QCOMPARE(cursor->wasRemoved(), wasRemoved);
			QVERIFY(cursor->timestamp() >= _firstStamp);
			QVERIFY(cursor->timestamp() <= _lastStamp);

			hasNext = cursor->hasNext();
			QCOMPARE(cursor->next(), hasNext);
		} while(hasNext);
		QVERIFY(dataList.isEmpty());

		delete cursor;
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestEventCursor::testScanLog()
{
	try {
		auto cursor = EventCursor::first(this);

		bool finish = false;
		QList<std::tuple<quint64, ObjectKey, bool>> dataList {
			std::make_tuple(1, TestLib::generateKey(2), false),
			std::make_tuple(4, TestLib::generateKey(6), false),
			std::make_tuple(5, TestLib::generateKey(4), true)
		};

		//init the scan
		cursor->autoScanLog(this, [&](const EventCursor *scanCursor) {
			auto ok = false;
			[&](){
				QVERIFY(!dataList.isEmpty());
				quint64 index;
				ObjectKey key;
				bool wasRemoved;
				std::tie(index, key, wasRemoved) = dataList.takeFirst();
				//TODO c++17: auto[index, key, wasRemoved] = dataList.takeFirst();

				QCOMPARE(scanCursor, cursor);
				QCOMPARE(scanCursor->index(), index);
				QCOMPARE(scanCursor->key(), key);
				QCOMPARE(scanCursor->wasRemoved(), wasRemoved);
				QVERIFY(scanCursor->timestamp() >= _firstStamp);
				QVERIFY(scanCursor->timestamp() <= _lastStamp);

				QCOMPARE(scanCursor->hasNext(), !dataList.isEmpty());
				ok = true;
			}();
			return ok && !finish;
		});
		QVERIFY(dataList.isEmpty());

		DataTypeStore<TestData, int> store;
		//add some data
		dataList.append(std::make_tuple(6, TestLib::generateKey(8), false));
		_firstStamp = QDateTime::currentDateTime();
		store.save(TestLib::generateData(8));
		_lastStamp = QDateTime::currentDateTime();
		QTRY_VERIFY(dataList.isEmpty());

		//double add/remove data
		dataList.append(std::make_tuple(8, TestLib::generateKey(10), true));
		_firstStamp = QDateTime::currentDateTime();
		store.save(TestLib::generateData(10));
		store.remove(10);
		_lastStamp = QDateTime::currentDateTime();
		QTRY_VERIFY(dataList.isEmpty());

		//remove data and finish
		dataList.append(std::make_tuple(9, TestLib::generateKey(2), true));
		finish = true;
		_firstStamp = QDateTime::currentDateTime();
		store.remove(2);
		_lastStamp = QDateTime::currentDateTime();
		QTRY_VERIFY(dataList.isEmpty());

		//scan handler is stopped - next event should NOT trigger
		dataList.append(std::make_tuple(10, TestLib::generateKey(2), false));
		store.save(TestLib::generateData(2));
		QEXPECT_FAIL("", "No data changes on deactivated scan handler", Continue);
		QTRY_VERIFY(dataList.isEmpty());

		// make shure the event actually exists
		QVERIFY(cursor->hasNext());
		QVERIFY(cursor->next());
		QVERIFY(!cursor->hasNext());
		QCOMPARE(cursor->index(), 10);

		delete cursor;
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestEventCursor::testClearLog()
{
	try {
		auto cursor = EventCursor::last(this);
		QVERIFY(cursor);
		QVERIFY(cursor->isValid());
		QCOMPARE(cursor->index(), 10);
		QCOMPARE(cursor->key(), TestLib::generateKey(2));
		QCOMPARE(cursor->wasRemoved(), false);

		//invalid clear
		QVERIFY_EXCEPTION_THROWN(cursor->clearEventLog(11), EventCursorException);

		// normal state
		auto first = EventCursor::first(this);
		QVERIFY(first);
		QVERIFY(first->isValid());
		QCOMPARE(first->index(), 1);
		delete first;

		// delete with offset
		cursor->clearEventLog(5);
		first = EventCursor::first(this);
		QVERIFY(first);
		QVERIFY(first->isValid());
		QCOMPARE(first->index(), 5);
		delete first;

		// delete without offset
		cursor->clearEventLog();
		first = EventCursor::first(this);
		QVERIFY(first);
		QVERIFY(first->isValid());
		QCOMPARE(first->index(), 10);
		delete first;

		delete cursor;
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestEventCursor::testReset()
{
	try {
		LocalStore store{DefaultsPrivate::obtainDefaults(DefaultSetup)};

		store.reset(true);
		auto cursor = EventCursor::first(this);
		QVERIFY(cursor);
		QVERIFY(cursor->isValid());
		QCOMPARE(cursor->index(), 10);
		delete cursor;

		store.reset(false);
		cursor = EventCursor::first(this);
		QVERIFY(cursor);
		QVERIFY(!cursor->isValid());
		delete cursor;
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

QTEST_MAIN(TestEventCursor)

#include "tst_eventcursor.moc"
