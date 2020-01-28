#ifndef QTDATASYNC_FIREBASE_REALTIMED_APIBASE_P_H
#define QTDATASYNC_FIREBASE_REALTIMED_APIBASE_P_H

#include <QtCore/QObject>

namespace QtDataSync::firebase::realtimedb {

class ApiBase : public QObject
{
	Q_OBJECT

public:
	static const QByteArray NullETag;

	class ETagSetter
	{
		Q_DISABLE_COPY_MOVE(ETagSetter)
	public:
		ETagSetter(ApiBase *api, QByteArray eTag);
		~ETagSetter();

	private:
		ApiBase *_api;
	};

	explicit ApiBase(QObject *parent = nullptr);

protected:
	QByteArray getETag() const;

private:
	QByteArray _eTag;
};

}

#endif // QTDATASYNC_FIREBASE_REALTIMED_APIBASE_P_H
