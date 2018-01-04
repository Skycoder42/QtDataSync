#include "exchangebuffer_p.h"
using namespace QtDataSync;

Q_LOGGING_CATEGORY(rothreadedbackend, "qtdatasync.rothreadedbackend", QtWarningMsg)

ExchangeBuffer::ExchangeBuffer(QObject *parent) :
	QIODevice(parent),
	_partner(nullptr),
	_buffers(),
	_index(0),
	_size(0)
{}

ExchangeBuffer::~ExchangeBuffer()
{
	if(isOpen())
		close();
}

bool ExchangeBuffer::connectTo(ExchangeBuffer *partner, bool blocking)
{
	Q_ASSERT_X(partner, Q_FUNC_INFO, "partner must not be nullptr");
	if(_partner || isOpen()) {
		setErrorString(tr("ExchangeBuffer already open"));
		return false;
	} else {
		auto ok = QMetaObject::invokeMethod(partner, "openInteral",
											blocking ? Qt::BlockingQueuedConnection : Qt::QueuedConnection,
											Q_ARG(ExchangeBuffer*, this));
		return ok && openInteral(partner);
	}
}

bool ExchangeBuffer::isSequential() const
{
	return true;
}

void ExchangeBuffer::close()
{
	_buffers.clear();
	if(_partner) {
		QMetaObject::invokeMethod(_partner, "partnerClosed", Qt::QueuedConnection);
		_partner = nullptr;
	}
	QIODevice::close();
	emit partnerDisconnected({});
}

qint64 ExchangeBuffer::bytesAvailable() const
{
	return QIODevice::bytesAvailable() + _size;
}

qint64 ExchangeBuffer::readData(char *data, qint64 maxlen)
{
	qint64 written = 0;

	while(written < maxlen && !_buffers.isEmpty()) {
		auto buffer = _buffers.head();
		auto delta = qMin<qint64>(buffer.size() - _index, (maxlen - written));
		memcpy(data + written, buffer.constData() + _index, delta);
		written += delta;
		_index += static_cast<int>(delta);
		if(_index >= buffer.size()) {
			_buffers.dequeue();
			_index = 0;
		}
	}

	_size -= written;
	return written;
}

qint64 ExchangeBuffer::writeData(const char *data, qint64 len)
{
	if(len <= 0 || !_partner)
		return 0;

	if(QMetaObject::invokeMethod(_partner, "receiveData", Qt::QueuedConnection,
								 Q_ARG(QByteArray, QByteArray(data, len)))) {
		return len;
	} else {
		setErrorString(tr("Failed to send data to partner device"));
		return -1;
	}
}

bool ExchangeBuffer::openInteral(ExchangeBuffer *partner)
{
	Q_ASSERT_X(partner, Q_FUNC_INFO, "partner must not be nullptr");
	if(_partner || isOpen())
		qCWarning(rothreadedbackend) << "Connected to partner while already having a partner";
	_partner = partner;
	if(QIODevice::open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Unbuffered)) {
		emit partnerConnected({});
		return true;
	} else
		return false;
}

void ExchangeBuffer::receiveData(const QByteArray &data)
{
	Q_ASSERT_X(!data.isEmpty(), Q_FUNC_INFO, "receiveData called with an empty data bytearray");
	if(!isOpen())
		return;
	_buffers.enqueue(data);
	_size += data.size();
	emit readyRead();
}

void ExchangeBuffer::partnerClosed()
{
	if(_partner)
		_partner = nullptr;

	if(isOpen()) {
		_buffers.clear();
		QIODevice::close();
		emit partnerDisconnected({});
	}
}

bool ExchangeBuffer::open(QIODevice::OpenMode mode)
{
	Q_UNUSED(mode);
	return false;
}
