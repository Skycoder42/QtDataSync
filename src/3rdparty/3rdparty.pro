TEMPLATE = subdirs

system_cryptopp:unix: message("Not building cryptopp, using system provided version")
else: SUBDIRS += cryptopp

prepareRecursiveTarget(lrelease)
QMAKE_EXTRA_TARGETS += lrelease
