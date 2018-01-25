#include "threadedclient_p.h"
#include "threadedserver_p.h"

#include <chrono>
using namespace std::chrono;

#if QT_HAS_INCLUDE(<chrono>)
#define scdtime(x) x
#else
#define scdtime(x) duration_cast<milliseconds>(x).count()
#endif

using namespace QtDataSync;

ThreadedClientIoDevice::ThreadedClientIoDevice(QObject *parent) :
	ClientIoDevice(parent),
	_buffer(new ExchangeBuffer(this)),
	_connectTimer(new QTimer(this))
{
	connect(_buffer, &ExchangeBuffer::partnerConnected,
			this, &ThreadedClientIoDevice::deviceConnected);
	connect(_buffer, &ExchangeBuffer::readyRead,
			this, &ThreadedClientIoDevice::readyRead);
	connect(_buffer, &ExchangeBuffer::partnerDisconnected,
			this, &ThreadedClientIoDevice::deviceClosed);

	_connectTimer->setSingleShot(true);
	_connectTimer->setTimerType(Qt::VeryCoarseTimer);
	_connectTimer->setInterval(scdtime(seconds(30)));
	connect(_connectTimer, &QTimer::timeout,
			this, &ThreadedClientIoDevice::connectTimeout);
}

ThreadedClientIoDevice::~ThreadedClientIoDevice()
{
	_buffer->disconnect();
	if(!isClosing())
		close();
}

void ThreadedClientIoDevice::connectToServer()
{
	if(isOpen() || !url().isValid())
		return;
	if(_buffer->isOpen())
		_buffer->close();

	if(url().scheme() != ThreadedServer::UrlScheme()) {
		qCCritical(rothreadedbackend).noquote() << "Unsupported URL-Scheme:" << url().scheme();
		return;
	}
	if(ThreadedServer::connectTo(url(), _buffer))
		_connectTimer->start();
	else
		emit shouldReconnect(this);
}

bool ThreadedClientIoDevice::isOpen()
{
	return !isClosing() && _buffer->isOpen();
}

QIODevice *ThreadedClientIoDevice::connection()
{
	return _buffer;
}

void ThreadedClientIoDevice::doClose()
{
	if(_buffer->isOpen())
		_buffer->close();
	deleteLater();
}

void ThreadedClientIoDevice::deviceConnected()
{
	_connectTimer->stop();
	m_dataStream.setDevice(connection());
	m_dataStream.resetStatus();
}

void ThreadedClientIoDevice::deviceClosed()
{
	if(!isClosing()) {
		qCWarning(rothreadedbackend) << "Partner device closed connection unexpected";
		emit shouldReconnect(this);
	}
}

void ThreadedClientIoDevice::connectTimeout()
{
	qCWarning(rothreadedbackend) << "Connecting to partner device timed out";
	emit shouldReconnect(this);
}
