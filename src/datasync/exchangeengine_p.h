#ifndef EXCHANGEENGINE_P_H
#define EXCHANGEENGINE_P_H

#include <QtCore/QObject>

#include "qtdatasync_global.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT ExchangeEngine : public QObject
{
	Q_OBJECT

public:
	explicit ExchangeEngine(QObject *parent = nullptr);

	void enterFatalState(const QString &error);

public Q_SLOTS:
	void initialize();
	void finalize();
};

}

#endif // EXCHANGEENGINE_P_H
