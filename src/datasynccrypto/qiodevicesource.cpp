#include "qiodevicesource.h"
using namespace QtDataSync::Crypto;
using namespace CryptoPP;

namespace QtDataSync::Crypto {

struct QIODeviceStorePrivate {
	QIODevice *device = nullptr;
	CryptoPP::byte *space = nullptr;
	qint64 len = 0;
	bool waiting = false;
};

struct QByteArraySourcePrivate {
	QByteArray data {};
	QBuffer buffer {};
};

}

const char * const QIODeviceStore::DeviceParameter = "QIODevice";

QIODeviceStore::QIODeviceStore() :
	d{new QIODeviceStorePrivate{}}
{}

QIODeviceStore::QIODeviceStore(QIODevice *device) :
	QIODeviceStore{}
{
	StoreInitialize(MakeParameters(DeviceParameter, device));
}

lword QIODeviceStore::MaxRetrievable() const
{
	if (!d->device)
		return 0;
	return d->device->bytesAvailable();
}

size_t QIODeviceStore::TransferTo2(BufferedTransformation &target, lword &transferBytes, const std::string &channel, bool blocking)
{
	if (!d->device) {
		transferBytes = 0;
		return 0;
	}

	auto size = transferBytes;
	transferBytes = 0;

	while (size > 0 && !d->device->atEnd()) {
		if (!d->waiting) {
			size_t spaceSize = 1024;
			d->space = HelpCreatePutSpace(target,
										  channel,
										  1,
										  UnsignedMin(static_cast<size_t>(SIZE_MAX), size),
										  spaceSize);
			d->len = static_cast<size_t>(
				d->device->read(reinterpret_cast<char*>(d->space),
								static_cast<qint64>(STDMIN(size, static_cast<lword>(spaceSize)))));
			if (d->len == -1) {
				throw Err{
					QStringLiteral("QIODeviceStore: Failed to read with error: %1")
						.arg(d->device->errorString())
				};
			}
		}

		const auto blockedBytes = target.ChannelPutModifiable2(channel, d->space, d->len, 0, blocking);
		d->waiting = blockedBytes > 0;
		if (d->waiting)
			return blockedBytes;
		size -= static_cast<size_t>(d->len);
		transferBytes += static_cast<size_t>(d->len);
	}

	return 0;
}

size_t QIODeviceStore::CopyRangeTo2(BufferedTransformation &target, lword &begin, lword end, const std::string &channel, bool blocking) const
{
	if (!d->device)
		return 0;

	QByteArray data;
	if (d->device->isSequential()) {
		data = d->device->peek(static_cast<qint64>(end));
		if (data.isEmpty())
			return 0;
		data = data.mid(static_cast<int>(begin));
	} else {
		const auto cPos = d->device->pos();
		const auto nPos = cPos + static_cast<qint64>(begin);
		if (nPos >= d->device->size())
			return 0;
		d->device->seek(nPos);
		data = d->device->read(static_cast<qint64>(end - begin));
		d->device->seek(cPos);
	}

	const auto blockedBytes = target.ChannelPut(channel,
												reinterpret_cast<byte*>(data.data()),
												static_cast<size_t>(data.size()),
												blocking);
	begin += static_cast<size_t>(data.size()) - blockedBytes;
	return blockedBytes;
}

lword QIODeviceStore::Skip(lword skipMax)
{
	if (!d->device)
		return 0;

	auto pos = d->device->pos();
	qint64 offset;
	if (!SafeConvert(skipMax, offset))  // TODO use safe convert everywhere
		throw InvalidArgument{"QIODeviceStore: maximum seek offset exceeded"};
	d->device->seek(pos + offset);
	return static_cast<lword>(d->device->pos() - pos);
}

QIODevice *QIODeviceStore::device() const
{
	return d->device;
}

void QIODeviceStore::StoreInitialize(const CryptoPP::NameValuePairs &parameters)
{
	d->device = nullptr;
	parameters.GetValue(DeviceParameter, d->device);
	if(!d->device || !d->device->isReadable())
		throw Err{QStringLiteral("QIODeviceStore: passed QIODevice is either null or not opened for reading")};
}



QIODeviceSource::QIODeviceSource(BufferedTransformation *attachment) :
	SourceTemplate<QIODeviceStore>{attachment}
{}

QIODeviceSource::QIODeviceSource(QIODevice *device, bool pumpAll, BufferedTransformation *attachment) :
	SourceTemplate<QIODeviceStore>{attachment}
{
	SourceInitialize(pumpAll, MakeParameters(QIODeviceStore::DeviceParameter, device));
}

QIODevice *QIODeviceSource::device() const
{
	return m_store.device();
}



QByteArraySource::QByteArraySource(BufferedTransformation *attachment) :
	QIODeviceSource{attachment},
	d{new QByteArraySourcePrivate{}}
{}

QByteArraySource::QByteArraySource(QByteArray source, bool pumpAll, BufferedTransformation *attachment) :
	QIODeviceSource{attachment},
	d{new QByteArraySourcePrivate{std::move(source)}}
{
	d->buffer.setBuffer(&d->data);
	d->buffer.open(QIODevice::ReadOnly);
	SourceInitialize(pumpAll, MakeParameters(QIODeviceStore::DeviceParameter, static_cast<QIODevice*>(&d->buffer)));
}

const QByteArray &QByteArraySource::buffer() const
{
	return d->data;
}



QIODeviceStore::Err::Err(const QString &s) :
	 Exception(IO_ERROR, s.toStdString())
{}
