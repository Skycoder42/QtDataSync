#include "message_p.h"

#include <qiodevicesource.h>
#include <qiodevicesink.h>

#include <QtCore/QMetaProperty>

using namespace QtDataSync;

void QtDataSync::setupStream(QDataStream &stream)
{
	static_assert(QDataStream::Qt_DefaultCompiledVersion == QDataStream::Qt_5_6, "update DS version");
	stream.setVersion(QDataStream::Qt_5_6);
	stream.resetStatus();
}

QByteArray QtDataSync::signData(RsaScheme::Signer &signer, CryptoPP::RandomNumberGenerator &rng, const QByteArray &message)
{
	QByteArray signature;
	QByteArraySource (message, true,
		new CryptoPP::SignerFilter(rng, signer,
			new QByteArraySink(signature)
	   ) // SignerFilter
	); // QByteArraySource
	return signature;
}

void QtDataSync::verifyData(RsaScheme::Verifier &verifier, const QByteArray &message, const QByteArray &signature)
{
	QByteArraySource (message + signature, true,
		new CryptoPP::SignatureVerificationFilter(
			verifier, nullptr,
			CryptoPP::SignatureVerificationFilter::THROW_EXCEPTION | CryptoPP::SignatureVerificationFilter::SIGNATURE_AT_END
		) // SignatureVerificationFilter
	); // StringSource
}

void QtDataSync::verifySignature(QDataStream &stream, const CryptoPP::RSA::PublicKey &pubKey)
{
	auto device = stream.device();
	auto cPos = device->pos();

	stream.startTransaction();
	QByteArray signature;
	stream >> signature;
	if(!stream.commitTransaction())
		throw DataStreamException(stream);

	auto nPos = device->pos();
	device->reset();
	auto msgData = device->read(cPos);
	device->seek(nPos);

	RsaScheme::Verifier verifier(pubKey);
	verifyData(verifier, msgData, signature);
}



DataStreamException::DataStreamException(QDataStream &stream) :
	_status(stream.status())
{
	stream.resetStatus();
}

DataStreamException::DataStreamException(QDataStream::Status status) :
	_status(status)
{}

const char *DataStreamException::what() const noexcept
{
	switch (_status) {
	case QDataStream::Ok:
		return "Unknown Error";
	case QDataStream::ReadPastEnd:
		return "Incomplete message received";
	case QDataStream::ReadCorruptData:
		return "Invalid message received";
	case QDataStream::WriteFailed:
		return "Writing message failed";
	default:
		Q_UNREACHABLE();
		break;
	}
}

void DataStreamException::raise() const
{
	throw (*this);
}

QException *DataStreamException::clone() const
{
	return new DataStreamException(_status);
}
