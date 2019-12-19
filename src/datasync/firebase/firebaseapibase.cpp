#include "firebaseapibase_p.h"
#include "servertimestamp_p.h"
using namespace QtDataSync::firebase;

FirebaseApiBase::FirebaseApiBase(QObject *parent) :
	QObject{parent}
{}

QByteArray FirebaseApiBase::getETag() const
{
	return _eTag;
}



FirebaseApiBase::ETagSetter::ETagSetter(FirebaseApiBase *api, QByteArray eTag) :
	_api{api}
{
	_api->_eTag = std::move(eTag);
}

FirebaseApiBase::ETagSetter::~ETagSetter()
{
	_api->_eTag.clear();
}
