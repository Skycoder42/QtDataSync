#ifndef QTDATASYNC_CONFLICTRESOLVER_P_H
#define QTDATASYNC_CONFLICTRESOLVER_P_H

#include "qtdatasync_global.h"
#include "conflictresolver.h"

namespace QtDataSync {

//no export needed
class ConflictResolverPrivate
{
public:
	ConflictResolverPrivate();

	Defaults defaults;
	Logger *logger;
	QSettings *settings;
};

}

#endif // QTDATASYNC_CONFLICTRESOLVER_P_H
