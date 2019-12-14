#include "exception.h"
using namespace QtDataSync;

const char *Exception::what() const noexcept
{
	if (!_qWhat)
		_qWhat = qWhat().toUtf8();
	return _qWhat->constData();
}

void Exception::raise() const
{
	Q_ASSERT_X(false, Q_FUNC_INFO, "unimplemented");
	std::abort();
}

QtDataSync::ExceptionBase *Exception::clone() const
{
	Q_ASSERT_X(false, Q_FUNC_INFO, "unimplemented");
	return nullptr;
}



FileException::FileException(const QIODevice *device) :
	_fileName{QStringLiteral("<unknown>")},
	_errorString{device->errorString()}
{}

FileException::FileException(const QFileDevice *device) :
	_fileName{device->fileName()},
	_errorString{device->errorString()}
{}

QString FileException::fileName() const
{
	return _fileName;
}

QString FileException::errorString() const
{
	return _errorString;
}

QString FileException::qWhat() const
{
	return _fileName + QStringLiteral(": ") + _errorString;
}

void FileException::raise() const
{
	throw *this;
}

QtDataSync::ExceptionBase *FileException::clone() const
{
	return new FileException{*this};
}



JsonException::JsonException(QJsonParseError error, const QIODevice *device) :
	FileException{device},
	_error{std::move(error)}
{
	_errorString = _error.errorString();
}

JsonException::JsonException(QJsonParseError error, const QFileDevice *device) :
	FileException{device},
	_error{std::move(error)}
{
	_errorString = _error.errorString();
}

int JsonException::offset() const
{
	return _error.offset;
}

QJsonParseError::ParseError JsonException::error() const
{
	return _error.error;
}

QString JsonException::qWhat() const
{
	return _fileName +
		   QStringLiteral(":") +
		   QString::number(_error.offset) +
		   QStringLiteral(": ") +
		   _errorString;
}

void JsonException::raise() const
{
	throw *this;
}

QtDataSync::ExceptionBase *JsonException::clone() const
{
	return new JsonException{*this};
}
