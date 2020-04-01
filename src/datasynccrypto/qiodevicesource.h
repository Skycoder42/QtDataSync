#ifndef QTDATASYNC_CRYPTO_QIODEVICESOURCE_H
#define QTDATASYNC_CRYPTO_QIODEVICESOURCE_H

#include "QtDataSyncCrypto/qtdatasynccrypto_global.h"

#include <QtCore/qiodevice.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qbuffer.h>
#include <QtCore/qscopedpointer.h>

#include <cryptopp/filters.h>

namespace QtDataSync::Crypto {

struct QIODeviceStorePrivate;
class Q_DATASYNC_CRYPTO_EXPORT QIODeviceStore :
  public CryptoPP::Store,
  private CryptoPP::FilterPutSpaceHelper,
  public CryptoPP::NotCopyable
{
public:
	static const char * const DeviceParameter;

	class Q_DATASYNC_CRYPTO_EXPORT Err : public CryptoPP::Exception
	{
	public:
		Err(const QString &s);
	};

	QIODeviceStore();
	QIODeviceStore(QIODevice *device);

	CryptoPP::lword MaxRetrievable() const override;
	size_t TransferTo2(CryptoPP::BufferedTransformation &target, CryptoPP::lword &transferBytes, const std::string &channel, bool blocking) override;
	size_t CopyRangeTo2(CryptoPP::BufferedTransformation &target, CryptoPP::lword &begin, CryptoPP::lword end, const std::string &channel, bool blocking) const override;
	CryptoPP::lword Skip(CryptoPP::lword skipMax = std::numeric_limits<CryptoPP::lword>::max()) override;

	QIODevice *device() const;

protected:
	void StoreInitialize(const CryptoPP::NameValuePairs &parameters) override;

private:
	QScopedPointer<QIODeviceStorePrivate> d;
};

class Q_DATASYNC_CRYPTO_EXPORT QIODeviceSource : public CryptoPP::SourceTemplate<QIODeviceStore>
{
	Q_DISABLE_COPY(QIODeviceSource)
public:
	QIODeviceSource(CryptoPP::BufferedTransformation *attachment = nullptr);
	QIODeviceSource(QIODevice *device, bool pumpAll, CryptoPP::BufferedTransformation *attachment = nullptr);

	QIODevice *device() const;
};

struct QByteArraySourcePrivate;
class Q_DATASYNC_CRYPTO_EXPORT QByteArraySource : public QIODeviceSource
{
	Q_DISABLE_COPY(QByteArraySource)
public:
	QByteArraySource(CryptoPP::BufferedTransformation *attachment = nullptr);
	QByteArraySource(QByteArray source, bool pumpAll, CryptoPP::BufferedTransformation *attachment = nullptr);

	const QByteArray &buffer() const;

private:
	QScopedPointer<QByteArraySourcePrivate> d;
};

}

#endif // QTDATASYNC_CRYPTO_QIODEVICESOURCE_H
