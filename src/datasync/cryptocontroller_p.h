#ifndef CRYPTOCONTROLLER_P_H
#define CRYPTOCONTROLLER_P_H

#include <QtCore/QObject>

#include "qtdatasync_global.h"
#include "defaults.h"
#include "keystore.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT CryptoController : public QObject
{
	Q_OBJECT

public:
	explicit CryptoController(const Defaults &defaults, QObject *parent = nullptr);

	static QStringList allKeys();

private:
	Defaults _defaults;
	Logger *_logger;
	QSettings *_settings;

	KeyStore *_keyStore;
};

}

#endif // CRYPTOCONTROLLER_P_H
