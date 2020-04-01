#ifndef QTDATASYNC_CRYPTO_QIODEVICESINK_H
#define QTDATASYNC_CRYPTO_QIODEVICESINK_H

#include "QtDataSyncCrypto/qtdatasynccrypto_global.h"

#include <QtCore/qiodevice.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qbuffer.h>
#include <QtCore/qscopedpointer.h>

#include <cryptopp/filters.h>

namespace QtDataSync::Crypto {

struct QIODeviceSinkPrivate;
class Q_DATASYNC_CRYPTO_EXPORT QIODeviceSink :
  public CryptoPP::Sink,
  public CryptoPP::NotCopyable
{
public:
	static const char * const DeviceParameter;

	class Q_DATASYNC_CRYPTO_EXPORT Err : public CryptoPP::Exception
	{
	public:
		Err(const QString &s);
	};

	QIODeviceSink();
	QIODeviceSink(QIODevice *device);

	QIODevice *device() const;

	void IsolatedInitialize(const CryptoPP::NameValuePairs &parameters) override;
	size_t Put2(const CryptoPP::byte *inString, size_t length, int messageEnd, bool blocking) override;
	bool IsolatedFlush(bool hardFlush, bool blocking) override;

private:
	QScopedPointer<QIODeviceSinkPrivate> d;
};

struct QByteArraySinkPrivate;
class Q_DATASYNC_CRYPTO_EXPORT QByteArraySink : public QIODeviceSink
{
public:
	QByteArraySink();
	QByteArraySink(QByteArray &sink);

	const QByteArray &buffer() const;

private:
	QScopedPointer<QByteArraySinkPrivate> d;
};

}

#endif // QTDATASYNC_CRYPTO_QIODEVICESINK_H
