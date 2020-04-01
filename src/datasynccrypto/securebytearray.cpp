#include "securebytearray.h"
#ifdef Q_OS_UNIX
#include <sys/mman.h>
#elif defined(Q_OS_WINDOWS)
#include <qt_windows.h>
#endif
using namespace QtDataSync::Crypto;
using namespace CryptoPP;

namespace QtDataSync::Crypto {

class SecureByteArrayData : public QSharedData
{
public:
	SecureByteArrayData();
	SecureByteArrayData(size_t size, const byte *copyData = nullptr);
	SecureByteArrayData(const SecureByteArrayData &other);
	SecureByteArrayData(SecureByteArrayData &&other) noexcept;
	~SecureByteArrayData();

	size_t size;
	CryptoPP::byte *data;

private:
	void init(const byte *copyData);
};

}

SecureByteArray::SecureByteArray() :
	d{new SecureByteArrayData{}}
{}

SecureByteArray::SecureByteArray(const SecureByteArray &other) = default;

SecureByteArray::SecureByteArray(SecureByteArray &&other) noexcept = default;

SecureByteArray &SecureByteArray::operator=(const SecureByteArray &other) = default;

SecureByteArray &SecureByteArray::operator=(SecureByteArray &&other) noexcept = default;

SecureByteArray::~SecureByteArray() = default;

SecureByteArray::SecureByteArray(size_t size) :
	d{new SecureByteArrayData{size}}
{}

SecureByteArray::SecureByteArray(const byte *data, size_t size) :
	d{new SecureByteArrayData{size, data}}
{}

SecureByteArray::SecureByteArray(const char *data, int size) :
	SecureByteArray{reinterpret_cast<const byte*>(data), static_cast<size_t>(size)}
{}

bool SecureByteArray::isValid() const
{
	return d->data;
}

QtDataSync::Crypto::SecureByteArray::operator bool() const
{
	return d->data;
}

bool SecureByteArray::operator!() const
{
	return !d->data;
}

size_t SecureByteArray::size() const
{
	return d->size;
}

const byte *SecureByteArray::constData() const
{
	return d->data;
}

const byte *SecureByteArray::data() const
{
	return d->data;
}

byte *SecureByteArray::data()
{
	return d->data;
}

QByteArray SecureByteArray::toRaw() const
{
	if (d->data) {
		return QByteArray::fromRawData(reinterpret_cast<const char*>(d->data),
									   static_cast<int>(d->size));
	} else
		return {};
}

SecureByteArray SecureByteArray::fromRaw(const QByteArray &data)
{
	return SecureByteArray{data.data(), data.size()};
}




QtDataSync::Crypto::SecureByteArrayData::SecureByteArrayData() :
	QSharedData{},
	size{0},
	data{nullptr}
{}

SecureByteArrayData::SecureByteArrayData(size_t allocSize, const byte *copyData) :
	QSharedData{},
	size{allocSize},
	data{new byte[size]}
{
	init(copyData);
}

SecureByteArrayData::SecureByteArrayData(const QtDataSync::Crypto::SecureByteArrayData &other) :
	QSharedData{other},
	size{other.size},
	data{new byte[size]}
{
	init(other.data);
}

SecureByteArrayData::SecureByteArrayData(QtDataSync::Crypto::SecureByteArrayData &&other) noexcept :
	QSharedData{std::move(other)},
	size{other.size},
	data{other.data}
{
	other.data = nullptr;
}

QtDataSync::Crypto::SecureByteArrayData::~SecureByteArrayData()
{
	if (data) {
		memset_z(data, 0, size);
#ifdef Q_OS_UNIX
		munlock(data, size);
#elif defined(Q_OS_WINDOWS)
		VirtualUnlock(data, size);
#endif
		delete[] data;
	}
}

void QtDataSync::Crypto::SecureByteArrayData::init(const byte* copyData)
{
#ifdef Q_OS_UNIX
	mlock(data, size);
#elif defined(Q_OS_WINDOWS)
	VirtualLock(data, size);
#endif
	if (copyData)
		memcpy_s(data, size, copyData, size);
	else
		memset_z(data, 0, size);
}
