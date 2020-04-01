#include "keymanager.h"
#include "keymanager_p.h"
using namespace QtDataSync::Crypto;

KeyManager::KeyManager(QObject *parent) :
	QObject{*new KeyManagerPrivate{}, parent}
{}

void KeyProvider::loadKey(std::function<void (const SecureByteArray &)> loadedCallback, std::function<void ()> failureCallback)
{

}
