#include "accessmessage_p.h"
using namespace QtDataSync;

AccessMessage::AccessMessage() = default;

AccessMessage::AccessMessage(QString deviceName, QByteArray nonce, const QSharedPointer<CryptoPP::X509PublicKey> &signKey, const QSharedPointer<CryptoPP::X509PublicKey> &cryptKey, AsymmetricCrypto *crypto, QByteArray pNonce, QUuid partnerId, QByteArray macscheme, QByteArray cmac, QByteArray trustmac) :
	RegisterBaseMessage{std::move(deviceName),
						std::move(nonce),
						signKey,
						cryptKey,
						crypto},
	pNonce{std::move(pNonce)},
	partnerId{partnerId},
	macscheme{std::move(macscheme)},
	cmac{std::move(cmac)},
	trustmac{std::move(trustmac)}
{}

const QMetaObject *AccessMessage::getMetaObject() const
{
	return &staticMetaObject;
}
