#include "controller_p.h"

using namespace QtDataSync;
using namespace std::chrono;

#if QT_HAS_INCLUDE(<chrono>)
#define scdtime(x) x
#else
#define scdtime(x) duration_cast<milliseconds>(x).count()
#endif

#define QTDATASYNC_LOG _logger

QtDataSync::Controller::Controller(const QByteArray &name, Defaults defaults, QObject *parent) :
	QObject(parent),
	_defaults(defaults),
	_logger(defaults.createLogger(name, this)),
	_settings(defaults.createSettings(this, QString::fromUtf8(name))),
	_opTimer(new QTimer(this))
{
	connect(_opTimer, &QTimer::timeout,
			this, &Controller::onTimeout);
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

void Controller::beginOp(const minutes &interval, bool startIfNotRunning)
{
	if(!startIfNotRunning && !_opTimer->isActive())
		return;

#if QT_HAS_INCLUDE(<chrono>)
	if(_opTimer->remainingTimeAsDuration() < interval)
		_opTimer->start(interval);
#else
	if(_opTimer->remainingTime() < scdtime(interval))
		_opTimer->start(scdtime(interval));
#endif
	logDebug() << "Started or refreshed operation timeout";
}

void Controller::beginSpecialOp(const minutes &interval)
{
#if QT_HAS_INCLUDE(<chrono>)
	if(interval < _specialOp.remainingTimeAsDuration())
#else
	if(scdtime(interval) < _specialOp.remainingTime())
#endif
		_specialOp.setRemainingTime(scdtime(interval));
	beginOp(interval);
}

void Controller::endOp()
{
	_opTimer->stop();
	_specialOp.setDeadline(QDeadlineTimer::Forever);
	logDebug() << "Cleared operation timeout";
}

void Controller::onTimeout()
{
	if(_specialOp.hasExpired()) {
		_specialOp.setDeadline(QDeadlineTimer::Forever);
		emit specialOperationTimeout();
	} else
		emit operationTimeout();
}
