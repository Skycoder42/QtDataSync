#ifndef CONFLICTRESOLVER_P_H
#define CONFLICTRESOLVER_P_H

#include "qtdatasync_global.h"
#include "conflictresolver.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT ConflictResolverPrivate
{
public:
	ConflictResolverPrivate();

	Defaults defaults;
	Logger *logger;
	QSettings *settings;
};

}

#endif // CONFLICTRESOLVER_P_H
