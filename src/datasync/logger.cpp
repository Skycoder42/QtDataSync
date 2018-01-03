#include "logger.h"
#include "logger_p.h"
#include "setup_p.h"
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

void Logger::reportFatalError(const QString &error, const char *file, int line, const char *function)
{
	QMessageLogger logger(file, line, function, d->logCat.categoryName());
	logger.critical("%s", qUtf8Printable(error));

	try {
		SetupPrivate::engine(d->setupName)->enterFatalState(error, file, line, function, d->logCat.categoryName());
	} catch(...) {
		logger.fatal("%s", qUtf8Printable(error));
	}
}

void Logger::reportFatalError(const char *error, const char *file, int line, const char *function)
{
	reportFatalError(QString::fromUtf8(error), file, line, function);
}

// ------------- PRIVATE IMPLEMENTATION -------------

LoggerPrivate::LoggerPrivate(const QString &setupName, const QByteArray &subCategory) :
	setupName(setupName),
	catName("qtdatasync." + setupName.toUtf8() + "." + subCategory),
	logCat(catName.constData(), QtInfoMsg)
{}
