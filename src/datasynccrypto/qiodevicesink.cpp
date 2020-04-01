#include "qiodevicesink.h"
using namespace QtDataSync::Crypto;
using namespace CryptoPP;

namespace QtDataSync::Crypto {

struct QIODeviceSinkPrivate {
	QIODevice *device = nullptr;
};

struct QByteArraySinkPrivate {
	QBuffer buffer {};
};

}

const char * const QIODeviceSink::DeviceParameter = "QIODevice";

QIODeviceSink::QIODeviceSink() :
	d{new QIODeviceSinkPrivate{}}
{}

QIODeviceSink::QIODeviceSink(QIODevice *device) :
	QIODeviceSink{}
{
	IsolatedInitialize(MakeParameters(DeviceParameter, device));
}

QIODevice *QIODeviceSink::device() const
{
	return d->device;
}

void QIODeviceSink::IsolatedInitialize(const NameValuePairs &parameters)
{
	d->device = nullptr;
	parameters.GetValue(DeviceParameter, d->device);
	if (!d->device || !d->device->isWritable())
		throw Err{QStringLiteral("QIODeviceSink: passed QIODevice is either null or not opened for writing")};
}

bool QIODeviceSink::IsolatedFlush(bool hardFlush, bool blocking)
{
	Q_UNUSED(hardFlush)

	if (!d->device)
		throw Err{QStringLiteral("QIODeviceSink: passed QIODevice is null")};

	if (blocking) {
		if (d->device->bytesToWrite() > 0) {
			if (!d->device->waitForBytesWritten(-1)) {
				throw Err {
					QStringLiteral("QIODeviceSink: Failed to flush with error: %1")
						.arg(d->device->errorString())
				};
			};
		}
	}

	return false;
}

size_t QIODeviceSink::Put2(const byte *inString, size_t length, int messageEnd, bool blocking)
{
	Q_UNUSED(blocking)

	if (!d->device)
		throw Err{QStringLiteral("QIODeviceSink: passed QIODevice is null")};

	while (length > 0) {
		qint64 size;
		if (!SafeConvert(length, size))
			size = std::numeric_limits<qint64>::max();
		auto written = d->device->write(reinterpret_cast<const char *>(inString), size);
		if (written == -1) {
			throw Err {
				QStringLiteral("QIODeviceSink: Failed to write with error: %1")
					.arg(d->device->errorString())
			};
		}
		inString += written;
		length -= static_cast<size_t>(written);
	}

	if (messageEnd)
		IsolatedFlush(false, blocking);
	return 0;
}



QByteArraySink::QByteArraySink() :
	QIODeviceSink{},
	d{new QByteArraySinkPrivate{}}
{}

QByteArraySink::QByteArraySink(QByteArray &sink) :
	QIODeviceSink{},
	d{new QByteArraySinkPrivate{QBuffer{&sink}}}
{
	d->buffer.open(QIODevice::WriteOnly);
	IsolatedInitialize(MakeParameters(DeviceParameter, static_cast<QIODevice*>(&d->buffer)));
}

const QByteArray &QByteArraySink::buffer() const
{
	return d->buffer.data();
}



QIODeviceSink::Err::Err(const QString &s) :
	Exception{IO_ERROR, s.toStdString()}
{}
