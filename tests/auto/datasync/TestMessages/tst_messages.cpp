#include <QString>
#include <QtTest>
#include <QCoreApplication>

#include <QtDataSync/private/message_p.h>
#include <QtDataSync/private/accessmessage_p.h>
#include <QtDataSync/private/accountmessage_p.h>
#include <QtDataSync/private/changedmessage_p.h>
#include <QtDataSync/private/changemessage_p.h>
#include <QtDataSync/private/devicechangemessage_p.h>
#include <QtDataSync/private/devicekeysmessage_p.h>
#include <QtDataSync/private/devicesmessage_p.h>
#include <QtDataSync/private/errormessage_p.h>
#include <QtDataSync/private/grantmessage_p.h>
#include <QtDataSync/private/identifymessage_p.h>
#include <QtDataSync/private/keychangemessage_p.h>
#include <QtDataSync/private/loginmessage_p.h>
#include <QtDataSync/private/macupdatemessage_p.h>
#include <QtDataSync/private/newkeymessage_p.h>
#include <QtDataSync/private/proofmessage_p.h>
#include <QtDataSync/private/registermessage_p.h>
#include <QtDataSync/private/removemessage_p.h>
#include <QtDataSync/private/syncmessage_p.h>
#include <QtDataSync/private/welcomemessage_p.h>
#include <QtDataSync/private/cryptocontroller_p.h>

using namespace QtDataSync;

Q_DECLARE_METATYPE(QtDataSync::Message*)

class TestMessages : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testSerialization_data();
	void testSerialization();

	void testSignedSerialization_data();
	void testSignedSerialization();

private:
	ClientCrypto *crypto;

	void addSignedData();
	void addAllData();
	template <typename TMessage>
	void addData(std::function<TMessage()> createFn, bool success = true);
};

void TestMessages::initTestCase()
{
#ifdef Q_OS_LINUX
	if(!qgetenv("LD_PRELOAD").contains("Qt5DataSync"))
		qWarning() << "No LD_PRELOAD set - this may fail on systems with multiple version of the modules";
#endif
	qRegisterMetaType<QtDataSync::Message*>();
	QMetaType::registerComparators<Utf8String>();
	QMetaType::registerComparators<DevicesMessage::DeviceInfo>();
	QMetaType::registerComparators<QList<DevicesMessage::DeviceInfo>>();
	QMetaType::registerComparators<DeviceKeysMessage::DeviceKey>();
	QMetaType::registerComparators<QList<DeviceKeysMessage::DeviceKey>>();
	QMetaType::registerComparators<NewKeyMessage::KeyUpdate>();
	QMetaType::registerComparators<QList<NewKeyMessage::KeyUpdate>>();

	crypto = new ClientCrypto(this);
	crypto->generate(Setup::ECDSA_ECP_SHA3_512, Setup::brainpoolP256r1,
					 Setup::RSA_OAEP_SHA3_512, 2048);
}

void TestMessages::cleanupTestCase()
{
	crypto->deleteLater();
	crypto = nullptr;
}

void TestMessages::testSerialization_data()
{
	addAllData();
}

void TestMessages::testSerialization()
{
	QFETCH(QByteArray, name);
	QFETCH(QtDataSync::Message*, message);
	QFETCH(QtDataSync::Message*, resultMessage);
	QFETCH(bool, success);
	QFETCH(QByteArray, className);

	const Message &in = *message;
	Message &out = *resultMessage;

	try {
		QCOMPARE(QByteArray(in.metaObject()->className()), className);
		QCOMPARE(QByteArray(out.metaObject()->className()), className);
		QCOMPARE(in.messageName(), name);
		auto data = in.serialize();

		QDataStream stream(data);
		Message::setupStream(stream);
		QByteArray resName;
		stream >> resName;
		QCOMPARE(resName, name);

		if(success) {
			Message::deserializeMessageTo(stream, out);
			auto mo = message->metaObject();
			for(auto i = 0; i < mo->propertyCount(); i++) {
				auto p = mo->property(i);
				auto x1 = p.readOnGadget(message);
				auto x2 = p.readOnGadget(resultMessage);
				QCOMPARE(x2, x1);
			}
		} else
			QVERIFY_EXCEPTION_THROWN(Message::deserializeMessageTo(stream, out), DataStreamException);
	} catch (std::exception &e) {
		QFAIL(e.what());
	}

	delete message;
	delete resultMessage;
}

void TestMessages::testSignedSerialization_data()
{
	addSignedData();
}

void TestMessages::testSignedSerialization()
{
	QFETCH(QByteArray, name);
	QFETCH(QtDataSync::Message*, message);
	QFETCH(QtDataSync::Message*, resultMessage);
	QFETCH(bool, success);
	QFETCH(QByteArray, className);

	const Message &in = *message;
	Message &out = *resultMessage;

	try {
		QCOMPARE(QByteArray(in.metaObject()->className()), className);
		QCOMPARE(QByteArray(out.metaObject()->className()), className);
		QCOMPARE(in.messageName(), name);
		auto data = in.serializeSigned(crypto->privateSignKey(), crypto->rng(), crypto);

		QDataStream stream(data);
		Message::setupStream(stream);
		QByteArray resName;
		stream >> resName;
		QCOMPARE(resName, name);

		if(success) {
			Message::deserializeMessageTo(stream, out);
			Message::verifySignature(stream, crypto->signKey(), crypto);

			auto mo = message->metaObject();
			for(auto i = 0; i < mo->propertyCount(); i++) {
				auto p = mo->property(i);
				auto x1 = p.readOnGadget(message);
				auto x2 = p.readOnGadget(resultMessage);
				QCOMPARE(x2, x1);
			}
		} else
			QVERIFY_EXCEPTION_THROWN(Message::deserializeMessageTo(stream, out), DataStreamException);
	} catch (std::exception &e) {
		QFAIL(e.what());
	}

	delete message;
	delete resultMessage;
}

void TestMessages::addSignedData()
{
	QTest::addColumn<QByteArray>("name");
	QTest::addColumn<QtDataSync::Message*>("message");
	QTest::addColumn<QtDataSync::Message*>("resultMessage");
	QTest::addColumn<bool>("success");
	QTest::addColumn<QByteArray>("className");

	addData<RegisterMessage>([&]() {
		return RegisterMessage(QStringLiteral("devName"),
							   QByteArray(InitMessage::NonceSize, 'x'),
							   crypto->signKey(),
							   crypto->cryptKey(),
							   crypto,
							   "random cmac");
	});
	addData<LoginMessage>([&]() {
		return LoginMessage(QUuid::createUuid(),
							QStringLiteral("devName"),
							QByteArray(InitMessage::NonceSize, 'x'));
	});
	addData<LoginMessage>([&]() {
		return LoginMessage(QUuid::createUuid(),
							QStringLiteral("devName"),
							QByteArray(3, 'x'));
	}, false);
	addData<AccessMessage>([&]() {
		return AccessMessage(QStringLiteral("devName"),
							 QByteArray(InitMessage::NonceSize, 'x'),
							 crypto->signKey(),
							 crypto->cryptKey(),
							 crypto,
							 QByteArray(InitMessage::NonceSize, 'x'),
							 QUuid::createUuid(),
							 "macscheme",
							 "cmac",
							 "trustmac");
	});
	addData<AcceptMessage>([&]() {
		AcceptMessage msg(QUuid::createUuid());
		msg.index = 42;
		msg.scheme = "key_scheme";
		msg.secret = "encrypted_key";
		return msg;
	});
	addData<NewKeyMessage>([&]() {
		NewKeyMessage msg;
		msg.keyIndex = 33;
		msg.cmac = "key_cmac";
		msg.scheme = "random_scheme";
		msg.deviceKeys.append(std::make_tuple(
								  QUuid::createUuid(),
								  "data1",
								  "data2"
							  ));
		msg.deviceKeys.append(std::make_tuple(
								  QUuid::createUuid(),
								  "data4",
								  "data5"
							  ));
		return msg;

	});
}

void TestMessages::addAllData()
{
	addSignedData();

	addData<ErrorMessage>([&]() {
		return ErrorMessage(ErrorMessage::AuthenticationError,
							QStringLiteral("it is fatal!"),
							true);
	});

	addData<InitMessage>([&]() {
		InitMessage m;
		m.nonce = QByteArray(InitMessage::NonceSize, 'x');
		return m;
	});
	addData<InitMessage>([&]() {
		InitMessage m;
		m.nonce = QByteArray(3, 'y');
		return m;
	}, false);
	addData<IdentifyMessage>([&]() {
		return IdentifyMessage::createRandom(42, crypto->rng());
	});
	addData<RegisterBaseMessage>([&]() {
		return RegisterBaseMessage(QStringLiteral("devName"),
								   QByteArray(InitMessage::NonceSize, 'x'),
								   crypto->signKey(),
								   crypto->cryptKey(),
								   crypto);
	});
	addData<AccountMessage>([&]() {
		return AccountMessage(QUuid::createUuid());
	});
	addData<WelcomeMessage>([&]() {
		WelcomeMessage msg(true);
		msg.keyIndex = 42;
		msg.scheme = "scheme";
		msg.key = "key";
		msg.cmac = "cmac";
		return msg;
	});
	addData<MacUpdateMessage>([&]() {
		return MacUpdateMessage(42, "cmac");
	});
	addData<MacUpdateAckMessage>([&]() {
		return MacUpdateAckMessage();
	});

	addData<ChangeMessage>([&]() {
		ChangeMessage msg("id_hash");
		msg.keyIndex = 42;
		msg.salt = "random_salt";
		msg.data = "encrypted_data";
		return msg;
	});
	addData<ChangeAckMessage>([&]() {
		ChangeMessage msg("id_hash");
		msg.keyIndex = 42;
		msg.salt = "random_salt";
		msg.data = "encrypted_data";
		return ChangeAckMessage(msg);
	});

	addData<SyncMessage>([&]() {
		return SyncMessage();
	});
	addData<ChangedMessage>([&]() {
		ChangedMessage msg;
		msg.dataIndex = 77;
		msg.keyIndex = 42;
		msg.salt = "random_salt";
		msg.data = "encrypted_data";
		return msg;
	});
	addData<ChangedInfoMessage>([&]() {
		ChangedInfoMessage msg(11);
		msg.dataIndex = 77;
		msg.keyIndex = 42;
		msg.salt = "random_salt";
		msg.data = "encrypted_data";
		return msg;
	});
	addData<LastChangedMessage>([&]() {
		return LastChangedMessage();
	});
	addData<ChangedAckMessage>([&]() {
		return ChangedAckMessage(77);
	});

	addData<ProofMessage>([&]() {
		AccessMessage msg(QStringLiteral("devName"),
						  QByteArray(InitMessage::NonceSize, 'x'),
						  crypto->signKey(),
						  crypto->cryptKey(),
						  crypto,
						  QByteArray(InitMessage::NonceSize, 'x'),
						  QUuid::createUuid(),
						  "macscheme",
						  "cmac",
						  "trustmac");
		return ProofMessage(msg, QUuid::createUuid());
	});
	addData<DenyMessage>([&]() {
		return DenyMessage(QUuid::createUuid());
	});
	addData<GrantMessage>([&]() {
		AcceptMessage msg(QUuid::createUuid());
		msg.index = 42;
		msg.scheme = "key_scheme";
		msg.secret = "encrypted_key";
		return GrantMessage(msg);
	});
	addData<DeviceChangeMessage>([&]() {
		DeviceChangeMessage msg("id_hash", QUuid::createUuid());
		msg.keyIndex = 42;
		msg.salt = "random_salt";
		msg.data = "encrypted_data";
		return msg;
	});
	addData<DeviceChangeAckMessage>([&]() {
		DeviceChangeMessage msg("id_hash", QUuid::createUuid());
		msg.keyIndex = 42;
		msg.salt = "random_salt";
		msg.data = "encrypted_data";
		return DeviceChangeAckMessage(msg);
	});

	addData<ListDevicesMessage>([&]() {
		return ListDevicesMessage();
	});
	addData<DevicesMessage>([&]() {
		DevicesMessage msg;
		msg.devices.append(std::make_tuple(
							   QUuid::createUuid(),
							   QStringLiteral("device_name1"),
							   "device_fingerprint1"
						   ));
		msg.devices.append(std::make_tuple(
							   QUuid::createUuid(),
							   QStringLiteral("device_name2"),
							   "device_fingerprint2"
						   ));
		return msg;
	});

	addData<RemoveMessage>([&]() {
		return RemoveMessage(QUuid::createUuid());
	});
	addData<RemoveAckMessage>([&]() {
		return RemoveAckMessage(QUuid::createUuid());
	});

	addData<KeyChangeMessage>([&]() {
		return KeyChangeMessage(42);
	});
	addData<DeviceKeysMessage>([&]() {
		return DeviceKeysMessage(77);
	});
	addData<DeviceKeysMessage>([&]() {
		return DeviceKeysMessage(42, {
									 std::make_tuple(
										 QUuid::createUuid(),
										 "data1",
										 "data2",
										 "data3"
									 ), std::make_tuple(
										 QUuid::createUuid(),
										 "data4",
										 "data5",
										 "data6"
									 )
								 });
	});
	addData<NewKeyAckMessage>([&]() {
		NewKeyMessage msg;
		msg.keyIndex = 33;
		msg.cmac = "key_cmac";
		msg.scheme = "random_scheme";
		msg.deviceKeys.append(std::make_tuple(
								  QUuid::createUuid(),
								  "data1",
								  "data2"
							  ));
		msg.deviceKeys.append(std::make_tuple(
								  QUuid::createUuid(),
								  "data4",
								  "data5"
							  ));
		return NewKeyAckMessage(msg);
	});
}

template<typename TMessage>
void TestMessages::addData(std::function<TMessage()> createFn, bool success)
{
	auto m1 = new TMessage(createFn());
	auto m2 = new TMessage();
	auto name = m1->typeName();

	QTest::addRow(name.constData()) << m1->messageName()
									<< (Message*)m1
									<< (Message*)m2
									<< success
									<< QByteArray(TMessage::staticMetaObject.className());
}

QTEST_MAIN(TestMessages)

#include "tst_messages.moc"
