#include "accessmessage_p.h"
using namespace QtDataSync;

AccessMessage::AccessMessage() :
	RegisterBaseMessage()
{}

AccessMessage::AccessMessage(const QString &deviceName, const QByteArray &nonce, const QSharedPointer<CryptoPP::X509PublicKey> &signKey, const QSharedPointer<CryptoPP::X509PublicKey> &cryptKey, AsymmetricCrypto *crypto, const QByteArray &pNonce, const QUuid &partnerId, bool trusted, const QByteArray &signature) :
	RegisterBaseMessage(deviceName,
						nonce,
						signKey,
						cryptKey,
						crypto),
	pNonce(pNonce),
	partnerId(partnerId),
	trusted(trusted),
	signature(signature)
{}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const AccessMessage &message)
{
	stream << (RegisterBaseMessage)message
		   << message.pNonce
		   << message.partnerId
		   << message.trusted
		   << message.signature;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, AccessMessage &message)
{
	stream.startTransaction();
	stream >> (RegisterBaseMessage&)message
		   >> message.pNonce
		   >> message.partnerId
		   >> message.trusted
		   >> message.signature;
	stream.commitTransaction();
	return stream;
}
