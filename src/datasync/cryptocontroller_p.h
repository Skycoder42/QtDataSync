#ifndef CRYPTOCONTROLLER_P_H
#define CRYPTOCONTROLLER_P_H

#include <QtCore/QObject>
#include <QtCore/QUuid>

#include <cryptopp/osrng.h>
#include <cryptopp/rsa.h>

#include "qtdatasync_global.h"
#include "defaults.h"
#include "keystore.h"
#include "controller_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT CryptoController : public Controller
{
	Q_OBJECT

public:
	explicit CryptoController(const Defaults &defaults, QObject *parent = nullptr);

	static QStringList allKeys();

	void initialize() final;
	void finalize() final;

	bool canAccess() const;
	bool loadKeyMaterial(const QUuid &userId);

private:
	static const QString keyPKeyTemplate;

	KeyStore *_keyStore;

	CryptoPP::AutoSeededRandomPool _rng;
	CryptoPP::RSA::PrivateKey _privateKey;
};

}

#endif // CRYPTOCONTROLLER_P_H
