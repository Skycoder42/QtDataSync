#include "firebaseapibase_p.h"
#include "servertimestamp_p.h"
using namespace QtDataSync::firebase::realtimedb;

const QByteArray ApiBase::NullETag {"null_etag"};

ApiBase::ApiBase(QObject *parent) :
	QObject{parent}
{}

QByteArray ApiBase::getETag() const
{
	return _eTag;
}



ApiBase::ETagSetter::ETagSetter(ApiBase *api, QByteArray eTag) :
	_api{api}
{
	_api->_eTag = std::move(eTag);
}

ApiBase::ETagSetter::~ETagSetter()
{
	_api->_eTag = NullETag;
}
