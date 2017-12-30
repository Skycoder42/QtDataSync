#include "threadedclient_p.h"
using namespace QtDataSync;

ThreadedClientIoDevice::ThreadedClientIoDevice(QObject *parent) :
	ClientIoDevice(parent)
{}

void ThreadedClientIoDevice::connectToServer()
{

}

bool ThreadedClientIoDevice::isOpen()
{
	return false;
}

QIODevice *ThreadedClientIoDevice::connection()
{
	return nullptr;
}

void ThreadedClientIoDevice::doClose()
{

}
