#include "firebaseapibase_p.h"
using namespace QtDataSync::firebase;

FirebaseApiBase::FirebaseApiBase(QObject *parent) :
	QObject{parent}
{}

QString FirebaseApiBase::firebaseKey()
{
	return {};
}

QString FirebaseApiBase::firebaseProjectId()
{
	return {};
}

QString FirebaseApiBase::firebaseUserId()
{
	return {};
}

QString FirebaseApiBase::firebaseUserToken()
{
	return {};
}

QString FirebaseApiBase::firebaseReadTimeout()
{

}

QByteArray FirebaseApiBase::getETag() const
{

}
