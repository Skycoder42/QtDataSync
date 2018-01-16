#ifndef MOCKCONNECTION_H
#define MOCKCONNECTION_H

#include <QObject>
#include <QWebSocket>
#include <QtTest>

#include <QtDataSync/private/message_p.h>
#include <QtDataSync/private/cryptocontroller_p.h>
#include <QtDataSync/private/errormessage_p.h>
#include <QtDataSync/private/cryptocontroller_p.h>

class MockConnection : public QObject
{
	Q_OBJECT

public:
	explicit MockConnection(QWebSocket *socket, QObject *parent = nullptr);

	void sendBytes(const QByteArray &data);
	void send(const QtDataSync::Message &message);
	void sendSigned(const QtDataSync::Message &message, QtDataSync::ClientCrypto *crypto);
	void sendPing();
	void close();
	//server does not need signed sending

	bool waitForNothing();
	bool waitForPing();
	template <typename TMessage>
	bool waitForReply(const std::function<void(TMessage,bool&)> &fn);
	template <typename TMessage>
	bool waitForSignedReply(QtDataSync::ClientCrypto *crypto, const std::function<void(TMessage,bool&)> &fn);
	bool waitForError(QtDataSync::ErrorMessage::ErrorType type, bool recoverable = false);
	bool waitForDisconnect();

protected:
	QWebSocket *_socket;

private:
	QSignalSpy _msgSpy;
	QSignalSpy _closeSpy;
	bool _hasPing;

	bool waitForReplyImpl(const std::function<void(QByteArray,bool&)> &msgFn);
};



template<typename TMessage>
bool MockConnection::waitForReply(const std::function<void(TMessage, bool&)> &fn)
{
	return waitForReplyImpl([fn](QByteArray message, bool &ok) {
		QByteArray name;
		QDataStream stream(message);
		QtDataSync::Message::setupStream(stream);
		stream.startTransaction();
		stream >> name;
		if(!stream.commitTransaction())
			throw QtDataSync::DataStreamException(stream);

		QVERIFY2(QtDataSync::Message::isType<TMessage>(name), name.constData());
		fn(QtDataSync::Message::deserializeMessage<TMessage>(stream), ok);
	});
}

template<typename TMessage>
bool MockConnection::waitForSignedReply(QtDataSync::ClientCrypto *crypto, const std::function<void(TMessage, bool&)> &fn)
{
	return waitForReplyImpl([crypto, fn](QByteArray message, bool &ok) {
		QByteArray name;
		QDataStream stream(message);
		QtDataSync::Message::setupStream(stream);
		stream.startTransaction();
		stream >> name;
		if(!stream.commitTransaction())
			throw QtDataSync::DataStreamException(stream);

		QVERIFY(QtDataSync::Message::isType<TMessage>(name));
		auto msg = QtDataSync::Message::deserializeMessage<TMessage>(stream);
		QtDataSync::Message::verifySignature(stream, crypto->signKey(), crypto);
		fn(msg, ok);
	});
}

#endif // MOCKCONNECTION_H
