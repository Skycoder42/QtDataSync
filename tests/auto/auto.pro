TEMPLATE = subdirs

SUBDIRS += \
	datasync \
	cmake

cmake.CONFIG += no_run-tests_target
prepareRecursiveTarget(run-tests)
QMAKE_EXTRA_TARGETS += run-tests
