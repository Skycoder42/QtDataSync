#ifndef QTDATASYNC_FIREBASE_APIBASE_P_H
#define QTDATASYNC_FIREBASE_APIBASE_P_H

#include <QtCore/QObject>

namespace QtDataSync::firebase {

class FirebaseApiBase : public QObject
{
	Q_OBJECT

public:
	explicit FirebaseApiBase(QObject *parent = nullptr);

	static QString firebaseKey();
	static QString firebaseProjectId();
	static QString firebaseUserId();
	static QString firebaseUserToken();
	static QString firebaseReadTimeout();

protected:
	QByteArray getETag() const;
};

}

#endif // QTDATASYNC_FIREBASE_APIBASE_P_H
