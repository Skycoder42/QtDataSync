#ifndef QTDATASYNC_SETUP_P_H
#define QTDATASYNC_SETUP_P_H

#include "setup.h"
#include "authenticator.h"
#include "engine_p.h"

#include <QtCore/QLoggingCategory>

namespace QtDataSync {

class Q_DATASYNC_EXPORT SetupPrivate
{
public:
	EnginePrivate::OAuthConfig config;

	IAuthenticator *authenticator = nullptr;
};

Q_DECLARE_LOGGING_CATEGORY(logSetup)

}

#endif // QTDATASYNC_SETUP_P_H
