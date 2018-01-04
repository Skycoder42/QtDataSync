#include "qiodevicesink.h"
using namespace CryptoPP;

#ifdef max
#undef max
#endif

const char * const QIODeviceSink::DeviceParameter = "QIODevice";

QIODeviceSink::QIODeviceSink() :
	_device(nullptr)
{}

QIODeviceSink::QIODeviceSink(QIODevice *device) :
	_device(nullptr)
{
	IsolatedInitialize(MakeParameters(DeviceParameter, device));
}

QIODevice *QIODeviceSink::device() const
{
	return _device;
}

void QIODeviceSink::IsolatedInitialize(const NameValuePairs &parameters)
{
	_device = nullptr;
	parameters.GetValue(DeviceParameter, _device);
	if(!_device || !_device->isWritable())
		throw Err(QStringLiteral("QIODeviceSink: passed QIODevice is either null or not opened for writing"));
}

bool QIODeviceSink::IsolatedFlush(bool hardFlush, bool blocking)
{
	Q_UNUSED(hardFlush)
	Q_UNUSED(blocking)

	if(!_device)
		throw Err(QStringLiteral("QIODeviceSink: passed QIODevice is null"));

	if(_device->bytesToWrite() > 0) {
		if(!_device->waitForBytesWritten(-1))
			throw Err(QStringLiteral("QIODeviceSink: Failed to flush with error: %1").arg(_device->errorString()));
	}
	return false;
}

size_t QIODeviceSink::Put2(const byte *inString, size_t length, int messageEnd, bool blocking)
{
	Q_UNUSED(blocking)

	if (!_device)
		throw Err(QStringLiteral("QIODeviceSink: passed QIODevice is null"));

	while (length > 0)
	{
		qint64 size;
		if (!SafeConvert(length, size))
			size = std::numeric_limits<qint64>::max();
		auto written = _device->write(reinterpret_cast<const char *>(inString), size);
		if(written == -1)
			throw Err(QStringLiteral("QIODeviceSink: Failed to write with error: %1").arg(_device->errorString()));
		inString += written;
		length -= static_cast<size_t>(written);
	}

	if(messageEnd)
		IsolatedFlush(false, blocking);
	return 0;
}



QByteArraySink::QByteArraySink() :
	QIODeviceSink()
{}

QByteArraySink::QByteArraySink(QByteArray &sink) :
	QIODeviceSink(),
	_buffer(&sink)
{
	_buffer.open(QIODevice::WriteOnly);
	IsolatedInitialize(MakeParameters(DeviceParameter, static_cast<QIODevice*>(&_buffer)));
}

const QByteArray &QByteArraySink::buffer() const
{
	return _buffer.data();
}



QIODeviceSink::Err::Err(const QString &s) :
	Exception(IO_ERROR, s.toStdString())
{}
