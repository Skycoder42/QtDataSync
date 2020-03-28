#include "securebytearray.h"
using namespace QtDataSync::Crypto;
using namespace CryptoPP;

SecureByteArray::SecureByteArray(const QByteArray &other) :
	QByteArray{other}
{}

SecureByteArray::SecureByteArray(QByteArray &&other) noexcept :
	QByteArray{std::move(other)}
{}

SecureByteArray &SecureByteArray::operator=(const QByteArray &other)
{
	QByteArray::operator=(other);
	return *this;
}

SecureByteArray &SecureByteArray::operator=(QByteArray &&other) noexcept
{
	QByteArray::operator=(std::move(other));
	return *this;
}

SecureByteArray::SecureByteArray(const char *data, int size) :
	QByteArray{data, size}
{}

SecureByteArray::SecureByteArray(int size, char c) :
	QByteArray{size, c}
{}

SecureByteArray::SecureByteArray(int size, Qt::Initialization i) :
	QByteArray{size, i}
{}

SecureByteArray::SecureByteArray(const byte *data, size_t size) :
	QByteArray{reinterpret_cast<const char*>(data), static_cast<int>(size)}
{}

SecureByteArray::SecureByteArray(size_t size, CryptoPP::byte b) :
	QByteArray{static_cast<int>(size), static_cast<char>(b)}
{}

SecureByteArray::SecureByteArray(size_t size, Qt::Initialization i) :
	QByteArray{static_cast<int>(size), i}
{}

SecureByteArray SecureByteArray::fromRawByteData(const byte *data, size_t size)
{
	return QByteArray::fromRawData(reinterpret_cast<const char*>(data), static_cast<int>(size));
}

byte *SecureByteArray::byteData()
{
	return reinterpret_cast<byte*>(QByteArray::data());
}

const byte *SecureByteArray::byteData() const
{
	return reinterpret_cast<const byte*>(QByteArray::data());
}

const byte *SecureByteArray::constByteData() const
{
	return reinterpret_cast<const byte*>(QByteArray::constData());
}

size_t SecureByteArray::byteSize() const
{
	return static_cast<size_t>(QByteArray::size());
}

SecureByteArray::operator byte *()
{
	return reinterpret_cast<byte*>(QByteArray::data());
}

SecureByteArray::operator const byte *() const
{
	return reinterpret_cast<const byte*>(QByteArray::data());
}

