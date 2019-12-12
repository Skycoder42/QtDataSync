#ifndef QTDATASYNC_ENGINE_P_H
#define QTDATASYNC_ENGINE_P_H

#include "engine.h"
#include "firebaseapibase_p.h"
#include "authenticator.h"

#include <QtCore/private/qobject_p.h>

namespace QtDataSync {

class Q_DATASYNC_EXPORT EnginePrivate : public QObjectPrivate
{
	Q_DECLARE_PUBLIC(Engine)
public:
	struct FirebaseConfig {
		QString projectId;
		QString webApiKey;
	};

	struct OAuthConfig : public FirebaseConfig {
		QString clientId;
		QString secret;
		quint16 port = 0;
	};

	IAuthenticator *authenticator = nullptr;
};

}

#endif // QTDATASYNC_ENGINE_P_H
