#ifndef QTDATASYNC_SETUP_P_H
#define QTDATASYNC_SETUP_P_H

#include "setup.h"
#include "authenticator.h"

#include <QtCore/QDir>
#include <QtCore/QLoggingCategory>

namespace QtDataSync {

class Q_DATASYNC_EXPORT SetupPrivate
{
public:
	struct {
		QString projectId;
		QString webApiKey;
	} firebase;

	struct {
		quint16 port = 0;
		QString clientId;
		QString secret;
	} oAuth;

	std::optional<QDir> localDir;
	QSettings *settings = nullptr;
	IAuthenticator *authenticator = nullptr;

	void finializeForEngine(Engine *engine);
};

Q_DECLARE_LOGGING_CATEGORY(logSetup)

}

#endif // QTDATASYNC_SETUP_P_H
