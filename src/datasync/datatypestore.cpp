#include "datatypestore.h"
using namespace QtDataSync;

DataTypeStoreBase::DataTypeStoreBase(QObject *parent) :
	QObject{parent}
{}

QString DataTypeStoreBase::setupName() const
{
	return store()->setupName();
}
