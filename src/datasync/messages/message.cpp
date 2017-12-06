#include "message_p.h"

#include <QtCore/QMetaProperty>

using namespace QtDataSync;

void QtDataSync::setupStream(QDataStream &stream)
{
	static_assert(QDataStream::Qt_DefaultCompiledVersion == QDataStream::Qt_5_6, "update DS version");
	stream.setVersion(QDataStream::Qt_5_6);
	stream.resetStatus();
}



DataStreamException::DataStreamException(QDataStream &stream) :
	_status(stream.status())
{
	stream.resetStatus();
}

DataStreamException::DataStreamException(QDataStream::Status status) :
	_status(status)
{}

const char *DataStreamException::what() const noexcept
{
	switch (_status) {
	case QDataStream::Ok:
		return "Unknown Error";
	case QDataStream::ReadPastEnd:
		return "Incomplete message received";
	case QDataStream::ReadCorruptData:
		return "Invalid message received";
	case QDataStream::WriteFailed:
		return "Writing message failed";
	default:
		Q_UNREACHABLE();
		break;
	}
}

void DataStreamException::raise() const
{
	throw (*this);
}

QException *DataStreamException::clone() const
{
	return new DataStreamException(_status);
}
