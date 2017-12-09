#ifndef CRYPTOQQ_H
#define CRYPTOQQ_H

#include <QtNetwork/QSslKey>
#include <cryptopp/rsa.h>

namespace CryptoQQ
{
QSslKey toQtFormat(const CryptoPP::RSA::PrivateKey &pKey);
CryptoPP::RSA::PrivateKey fromQtFormat(const QSslKey &key);
}

#endif // CRYPTOQQ_H
