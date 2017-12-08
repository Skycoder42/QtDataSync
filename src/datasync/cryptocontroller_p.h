#ifndef CRYPTOCONTROLLER_P_H
#define CRYPTOCONTROLLER_P_H

#include <QtCore/QObject>

#include "qtdatasync_global.h"
#include "controller_p.h"
#include "defaults.h"
#include "keystore.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT CryptoController : public Controller
{
	Q_OBJECT

public:
	explicit CryptoController(const Defaults &defaults, QObject *parent = nullptr);

	void initialize() final;

	static QStringList allKeys();

private:
	KeyStore *_keyStore;
};

}

#endif // CRYPTOCONTROLLER_P_H
