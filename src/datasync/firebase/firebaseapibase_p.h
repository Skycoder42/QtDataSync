#ifndef QTDATASYNC_FIREBASE_APIBASE_P_H
#define QTDATASYNC_FIREBASE_APIBASE_P_H

#include <QtCore/QObject>

namespace QtDataSync::firebase {

class FirebaseApiBase : public QObject
{
	Q_OBJECT

public:
	class ETagSetter
	{
		Q_DISABLE_COPY_MOVE(ETagSetter)
	public:
		ETagSetter(FirebaseApiBase *api, QByteArray eTag);
		~ETagSetter();

	private:
		FirebaseApiBase *_api;
	};

	explicit FirebaseApiBase(QObject *parent = nullptr);

protected:
	QByteArray getETag() const;

private:
	QByteArray _eTag;
};

}

#endif // QTDATASYNC_FIREBASE_APIBASE_P_H
