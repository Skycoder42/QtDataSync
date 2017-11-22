#include "logger.h"
#include "logger_p.h"
using namespace QtDataSync;

Logger::Logger(QByteArray subCategory, const QString &setupName, QObject *parent) :
	QObject(parent),
	d(new LoggerPrivate(setupName, subCategory))
{}

Logger::~Logger() {}

const QLoggingCategory &Logger::loggingCategory() const
{
	return d->logCat;
}

void Logger::reportFatalError(const QString &error, bool resyncRecoverable, const char *file, int line, const char *function)
{
	QMessageLogger(file, line, function, d->logCat.categoryName()).critical(qUtf8Printable(error));

	//TODO
//	auto engine = SetupPrivate::engine(d->setupName);
//	if(!engine)
//		return;
//	engine->enterFatalState(error, resyncRecoverable);
}

// ------------- PRIVATE IMPLEMENTATION -------------

LoggerPrivate::LoggerPrivate(const QString &setupName, const QByteArray &subCategory) :
	setupName(setupName),
	catName("qtdatasync." + setupName.toUtf8() + "." + subCategory),
	logCat(catName.constData(), QtWarningMsg)
{}
