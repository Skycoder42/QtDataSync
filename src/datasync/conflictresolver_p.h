#ifndef QTDATASYNC_CONFLICTRESOLVER_P_H
#define QTDATASYNC_CONFLICTRESOLVER_P_H

#include "qtdatasync_global.h"
#include "conflictresolver.h"

namespace QtDataSync {

//no export needed
class ConflictResolverPrivate
{
public:
	Defaults defaults;
	Logger *logger = nullptr;
	QSettings *settings = nullptr;
};

}

#endif // QTDATASYNC_CONFLICTRESOLVER_P_H
