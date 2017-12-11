#ifndef REGISTERMESSAGE_H
#define REGISTERMESSAGE_H

#include "message_p.h"

#include <functional>

#include <QScopedPointer>

#include <cryptopp/rsa.h>

namespace QtDataSync {

class Q_DATASYNC_EXPORT RegisterMessage
{
	Q_GADGET

	Q_PROPERTY(std::string keyAlgorithm MEMBER keyAlgorithm)
	Q_PROPERTY(QByteArray pubKey MEMBER pubKey)
	Q_PROPERTY(QString deviceName MEMBER deviceName)

public:
	RegisterMessage();

	template <typename TScheme>
	static RegisterMessage create(const QString &deviceName,
								  const typename TScheme::PublicKey &pubKey);

	std::string keyAlgorithm;
	QByteArray pubKey;
	QString deviceName;

	template <typename TScheme>
	typename TScheme::PublicKey getKey() const;
	void getKey(CryptoPP::X509PublicKey &key) const;
	void setKey(const CryptoPP::X509PublicKey &key);
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const RegisterMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, RegisterMessage &message);
Q_DATASYNC_EXPORT QDebug operator<<(QDebug debug, const RegisterMessage &message);

// ------------- Generic Implementation -------------

template<typename TScheme>
RegisterMessage RegisterMessage::create(const QString &deviceName, const typename TScheme::PublicKey &pubKey)
{
	static_assert(std::is_base_of<CryptoPP::X509PublicKey, typename TScheme::PublicKey>::value, "Only X509PublicKey keys are supported for now");
	RegisterMessage m;
	m.keyAlgorithm = TScheme::StaticAlgorithmName();
	m.setKey(pubKey);
	m.deviceName = deviceName;
	return m;
}

template<typename TScheme>
typename TScheme::PublicKey RegisterMessage::getKey() const
{
	static_assert(std::is_base_of<CryptoPP::X509PublicKey, typename TScheme::PublicKey>::value, "Only X509PublicKey keys are supported for now");
	if(TScheme::StaticAlgorithmName() != keyAlgorithm)
		throw CryptoPP::Exception(CryptoPP::Exception::INVALID_DATA_FORMAT, "Requests Algorithm does not match message algorithm");
	typename TScheme::PublicKey key;
	getKey(key);
	return key;
}

}

Q_DECLARE_METATYPE(QtDataSync::RegisterMessage)

#endif // REGISTERMESSAGE_H
