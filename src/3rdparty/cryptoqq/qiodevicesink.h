#ifndef QIODEVICESINK_H
#define QIODEVICESINK_H

#include <cryptopp/filters.h>
#include <QIODevice>
#include <QByteArray>
#include <QBuffer>

class QIODeviceSink : public CryptoPP::Sink, public CryptoPP::NotCopyable
{
public:
	class Err : public CryptoPP::Exception
	{
	public:
		Err(const QString &s);
	};

	QIODeviceSink();
	QIODeviceSink(QIODevice *device);

	QIODevice *device() const;

	void IsolatedInitialize(const CryptoPP::NameValuePairs &parameters) override;
	size_t Put2(const byte *inString, size_t length, int messageEnd, bool blocking) override;
	bool IsolatedFlush(bool hardFlush, bool blocking) override;

protected:
	static const char * const DeviceParameter;

private:
	QIODevice *_device;
};

class QByteArraySink : public QIODeviceSink
{
public:
	QByteArraySink();
	QByteArraySink(QByteArray &sink);

	const QByteArray &buffer() const;

private:
	QBuffer _buffer;
};

#endif // QIODEVICESINK_H
