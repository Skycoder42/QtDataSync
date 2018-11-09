TEMPLATE = subdirs

!cross_compile: SUBDIRS += appserver

prepareRecursiveTarget(lrelease)
QMAKE_EXTRA_TARGETS += lrelease run-tests
