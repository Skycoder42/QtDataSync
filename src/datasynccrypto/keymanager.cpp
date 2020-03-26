#include "keymanager.h"
#include "keymanager_p.h"
using namespace QtDataSync::Crypto;

KeyManager::KeyManager(QObject *parent) :
    QObject{*new KeyManagerPrivate{}, parent}
{}
