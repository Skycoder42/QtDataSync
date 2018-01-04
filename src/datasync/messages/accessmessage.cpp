#include "accessmessage_p.h"
using namespace QtDataSync;

AccessMessage::AccessMessage() :
	RegisterBaseMessage()
{}

AccessMessage::AccessMessage(const QString &deviceName, const QByteArray &nonce, const QSharedPointer<CryptoPP::X509PublicKey> &signKey, const QSharedPointer<CryptoPP::X509PublicKey> &cryptKey, AsymmetricCrypto *crypto, const QByteArray &pNonce, const QUuid &partnerId, const QByteArray &macscheme, const QByteArray &cmac, const QByteArray &trustmac) :
	RegisterBaseMessage(deviceName,
						nonce,
						signKey,
						cryptKey,
						crypto),
	pNonce(pNonce),
	partnerId(partnerId),
	macscheme(macscheme),
	cmac(cmac),
	trustmac(trustmac)
{}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const AccessMessage &message)
{
	stream << static_cast<RegisterBaseMessage>(message)
		   << message.pNonce
		   << message.partnerId
		   << message.macscheme
		   << message.cmac
		   << message.trustmac;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, AccessMessage &message)
{
	stream.startTransaction();
	stream >> static_cast<RegisterBaseMessage&>(message)
		   >> message.pNonce
		   >> message.partnerId
		   >> message.macscheme
		   >> message.cmac
		   >> message.trustmac;
	stream.commitTransaction();
	return stream;
}
