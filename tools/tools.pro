TEMPLATE = subdirs

SUBDIRS += appserver

prepareRecursiveTarget(lrelease)
QMAKE_EXTRA_TARGETS += lrelease
