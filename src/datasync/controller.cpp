#include "controller_p.h"

using namespace QtDataSync;

QtDataSync::Controller::Controller(const QByteArray &name, Defaults defaults, QObject *parent) :
	QObject(parent),
	_defaults(defaults),
	_logger(defaults.createLogger(name, this)),
	_settings(defaults.createSettings(this, QString::fromUtf8(name))),
	_opTimer(new QTimer(this))
{
	connect(_opTimer, &QTimer::timeout,
			this, &Controller::operationTimeout);
	_opTimer->setSingleShot(true);
	_opTimer->setTimerType(Qt::VeryCoarseTimer);
}

void Controller::initialize(const QVariantHash &params)
{
	Q_UNUSED(params)
}

void Controller::finalize() {}

Defaults Controller::defaults() const
{
	return _defaults;
}

Logger *Controller::logger() const
{
	return _logger;
}

QSettings *Controller::settings() const
{
	return _settings;
}

void Controller::beginOp(const std::chrono::minutes &interval)
{
	if(_opTimer->remainingTimeAsDuration() < interval)
		_opTimer->start(interval);
}

void Controller::endOp()
{
	_opTimer->stop();
}
