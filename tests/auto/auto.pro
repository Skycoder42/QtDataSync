TEMPLATE = subdirs

SUBDIRS += cmake \
	datasync \
	qml

qml.depends += datasync

cmake.CONFIG += no_run-tests_target
prepareRecursiveTarget(run-tests)
QMAKE_EXTRA_TARGETS += run-tests
