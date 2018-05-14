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

const QMetaObject *AccessMessage::getMetaObject() const
{
	return &staticMetaObject;
}
