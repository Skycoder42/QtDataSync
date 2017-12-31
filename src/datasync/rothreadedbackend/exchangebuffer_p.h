#ifndef EXCHANGEBUFFER_P_H
#define EXCHANGEBUFFER_P_H

#include <QtCore/QIODevice>
#include <QtCore/QQueue>
#include <QtCore/QLoggingCategory>
#include <QtCore/QPointer>

#include "qtdatasync_global.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT ExchangeBuffer : public QIODevice
{
	Q_OBJECT

public:
	explicit ExchangeBuffer(QObject *parent = nullptr);
	~ExchangeBuffer();

	bool connectTo(ExchangeBuffer *partner, bool blocking = false);

	bool isSequential() const override;
	void close() override;
	qint64 bytesAvailable() const override;

Q_SIGNALS:
	void partnerConnected(QPrivateSignal);
	void partnerDisconnected(QPrivateSignal);

protected:
	qint64 readData(char *data, qint64 maxlen) override;
	qint64 writeData(const char *data, qint64 len) override;

private Q_SLOTS:
	bool openInteral(ExchangeBuffer *partner);
	void receiveData(const QByteArray &data);
	void partnerClosed();

private:
	QPointer<ExchangeBuffer> _partner;

	QQueue<QByteArray> _buffers;
	int _index;
	quint64 _size;

	bool open(OpenMode mode) override;
};

}

Q_DECLARE_LOGGING_CATEGORY(rothreadedbackend)

#endif // EXCHANGEBUFFER_P_H
