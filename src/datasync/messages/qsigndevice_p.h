#ifndef QSIGNDEVICE_P_H
#define QSIGNDEVICE_P_H

#include <QtCore/QObject>

#include "message_p.h"

#include <qpipedevice.h>

namespace QtDataSync {

class Q_DATASYNC_EXPORT QSignDevice : public QPipeDevice
{
	Q_OBJECT

public:
	explicit QSignDevice(const CryptoPP::RSA::PrivateKey &key,
						 CryptoPP::RandomNumberGenerator *rng,
						 QObject *parent = nullptr);

protected:
	QByteArray process(QByteArray &&data) override;
	void end() override;

private:
	CryptoPP::RSA::PrivateKey _key;
	CryptoPP::RandomNumberGenerator *_rng;
	QByteArray _buffer;
};

}

#endif // QSIGNDEVICE_P_H
