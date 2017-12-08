#ifndef CRYPTOCONTROLLER_P_H
#define CRYPTOCONTROLLER_P_H

#include <QtCore/QObject>

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

private:
	KeyStore *_keyStore;
};

}

#endif // CRYPTOCONTROLLER_P_H
