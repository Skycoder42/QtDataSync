load(qt_parts)

SUBDIRS += doc

doxygen.target = doxygen
doxygen.CONFIG = recursive
doxygen.recurse_target = doxygen
doxygen.recurse += doc
QMAKE_EXTRA_TARGETS += doxygen

runtests.target = run-tests
runtests.CONFIG = recursive
runtests.recurse_target = run-tests
runtests.recurse += sub_tests sub_src sub_tools
QMAKE_EXTRA_TARGETS += runtests

include(modeling/modeling.pri)

DISTFILES += .qmake.conf \
	sync.profile
