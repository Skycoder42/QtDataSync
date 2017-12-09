#include "registermessage_p.h"

#include <qiodevicesink.h>
#include <qiodevicesource.h>

using namespace QtDataSync;

RegisterMessage::RegisterMessage(const CryptoPP::RSA::PublicKey &pubKey) :
	pubKey(pubKey)
{}

CryptoPP::RSA::PublicKey RegisterMessage::getPubKey() const
{
    return pubKey;
}

void RegisterMessage::setPubKey(const CryptoPP::RSA::PublicKey &value)
{
    pubKey = value;
}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const RegisterMessage &message)
{
    try {
        QByteArray outBuf;
        QByteArraySink sink(outBuf);
        message.pubKey.DEREncodePublicKey(sink);
		stream << outBuf;
	} catch(CryptoPP::Exception &e) {
		qWarning() << "Private key write error:" << e.what();
		stream.setStatus(QDataStream::WriteFailed);
	}
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, RegisterMessage &message)
{
	stream.startTransaction();
	try {
		QByteArray keyData;
		stream >> keyData;

		QByteArraySource source(keyData, true);
		message.pubKey.BERDecodePublicKey(source, false, 0);

		stream.commitTransaction();
	} catch(CryptoPP::Exception &e) {
		qWarning() << "Private key read error:" << e.what();
		stream.abortTransaction();
	}
	return stream;
}

QDebug QtDataSync::operator<<(QDebug debug, const RegisterMessage &message)
{
	QDebugStateSaver saver(debug);
	debug << message.pubKey;
	return debug;
}
