#include "exchangeengine_p.h"

#include <QtCore/QThread>

using namespace QtDataSync;

ExchangeEngine::ExchangeEngine(QObject *parent) :
	QObject(parent)
{}

void ExchangeEngine::enterFatalState(const QString &error)
{
	qDebug(Q_FUNC_INFO);
	Q_UNIMPLEMENTED();
}

void ExchangeEngine::initialize()
{
	qDebug(Q_FUNC_INFO);
	Q_UNIMPLEMENTED();
}

void ExchangeEngine::finalize()
{
	qDebug(Q_FUNC_INFO);
	Q_UNIMPLEMENTED();
	thread()->quit();
}
