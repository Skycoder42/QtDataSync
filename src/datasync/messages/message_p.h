#ifndef MESSAGE_P_H
#define MESSAGE_P_H

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QDataStream>
#include <QtCore/QException>

#ifdef BUILD_QDATASYNCSERVER
#define Q_DATASYNC_EXPORT
#else
#include "qtdatasync_global.h"
#endif

namespace QtDataSync {

class Q_DATASYNC_EXPORT DataStreamException : public QException
{
public:
	DataStreamException(QDataStream &stream);

	const char *what() const noexcept override;
	void raise() const override;
	QException *clone() const override;

private:
	DataStreamException(QDataStream::Status status);
	QDataStream::Status _status;
};

void Q_DATASYNC_EXPORT setupStream(QDataStream &stream);

template <typename TMessage>
void serializeMessage(QDataStream &stream, const TMessage &message, bool withName = true);
template <typename TMessage>
QByteArray serializeMessage(const TMessage &message);

template <typename TMessage>
TMessage deserializeMessage(QDataStream &stream);

template <typename... Args>
void applyMessage(const QByteArray &data, Args... args);

void inline applyMessageInteral(QDataStream &stream, const QByteArray &name);
template <typename TMessage, typename... Args>
void applyMessageInteral(QDataStream &stream, const QByteArray &name, const std::function<void(TMessage)> &fn, Args... args);

// ------------- Generic Implementation -------------

template <typename TMessage>
void serializeMessage(QDataStream &stream, const TMessage &message, bool withName)
{
	static_assert(std::is_void<typename TMessage::QtGadgetHelper>::value, "Only Q_GADGETS can be serialized");
	if(withName)
		stream << QByteArray(TMessage::staticMetaObject.className());
	stream << message;
	if(stream.status() != QDataStream::Ok)
		throw DataStreamException(stream);
}

template <typename TMessage>
QByteArray serializeMessage(const TMessage &message)
{
	static_assert(std::is_void<typename TMessage::QtGadgetHelper>::value, "Only Q_GADGETS can be serialized");
	QByteArray out;
	QDataStream stream(&out, QIODevice::WriteOnly);
	setupStream(stream);
	serializeMessage(stream, message, true);
	return out;
}

template <typename TMessage>
TMessage deserializeMessage(QDataStream &stream)
{
	static_assert(std::is_void<typename TMessage::QtGadgetHelper>::value, "Only Q_GADGETS can be serialized");
	stream.startTransaction();
	TMessage message;
	stream >> message;
	if(stream.commitTransaction())
		return message;
	else
		throw DataStreamException(stream);
}

template<typename... Args>
void applyMessage(const QByteArray &data, Args... args)
{
	QDataStream stream(data);

	stream.startTransaction();
	QByteArray name;
	stream >> name;
	if(!stream.commitTransaction())
		throw DataStreamException(stream);

	applyMessageInteral(stream, name, args...);
}

void inline applyMessageInteral(QDataStream &stream, const QByteArray &name)
{
	Q_UNUSED(name)
	throw DataStreamException(stream);//TODO real exception
}

template<typename TMessage, typename... Args>
void applyMessageInteral(QDataStream &stream, const QByteArray &name, const std::function<void (TMessage)> &fn, Args... args)
{
	static_assert(std::is_void<typename TMessage::QtGadgetHelper>::value, "Only Q_GADGETS can be serialized");
	if(name == TMessage::staticMetaObject.className())
		fn(deserializeMessage<TMessage>(stream));
	else
		applyMessageInteral(stream, name, args...);
}


}

#endif // MESSAGE_H
