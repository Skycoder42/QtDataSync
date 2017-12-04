#ifndef QIODEVICESOURCE_H
#define QIODEVICESOURCE_H

#include <cryptopp/filters.h>
#include <QIODevice>
#include <QByteArray>
#include <QBuffer>

class QIODeviceStore : public CryptoPP::Store, private CryptoPP::FilterPutSpaceHelper, public CryptoPP::NotCopyable
{
public:
	static const char * const DeviceParameter;

	class Err : public CryptoPP::Exception
	{
	public:
		Err(const QString &s);
	};

	QIODeviceStore();
	QIODeviceStore(QIODevice *device);

	CryptoPP::lword MaxRetrievable() const override;
	size_t TransferTo2(CryptoPP::BufferedTransformation &target, CryptoPP::lword &transferBytes, const std::string &channel, bool blocking) override;
	size_t CopyRangeTo2(CryptoPP::BufferedTransformation &target, CryptoPP::lword &begin, CryptoPP::lword end, const std::string &channel, bool blocking) const override;
	CryptoPP::lword Skip(CryptoPP::lword skipMax=ULONG_MAX) override;

	QIODevice *device() const;

protected:
	void StoreInitialize(const CryptoPP::NameValuePairs &parameters) override;

private:
	QIODevice *_device;
	byte *_space;
	qint64 _len;
	bool _waiting;
};

class QIODeviceSource : public CryptoPP::SourceTemplate<QIODeviceStore>
{
public:
	QIODeviceSource(CryptoPP::BufferedTransformation *attachment = nullptr);
	QIODeviceSource(QIODevice *device, bool pumpAll, CryptoPP::BufferedTransformation *attachment = nullptr);

	QIODevice *device() const;
};

class QByteArraySource : public QIODeviceSource
{
public:
	QByteArraySource(CryptoPP::BufferedTransformation *attachment = nullptr);
	QByteArraySource(const QByteArray &source, bool pumpAll, CryptoPP::BufferedTransformation *attachment = nullptr);

	const QByteArray &buffer() const;

private:
	QBuffer _buffer;
};

#endif // QIODEVICESOURCE_H
