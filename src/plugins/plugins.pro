TEMPLATE = subdirs

QT_FOR_CONFIG += core
SUBDIRS += keystores

prepareRecursiveTarget(lrelease)
QMAKE_EXTRA_TARGETS += lrelease
