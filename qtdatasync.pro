load(qt_parts)

SUBDIRS += doc

docTarget.target = doxygen
docTarget.CONFIG += recursive
docTarget.recurse_target = doxygen
QMAKE_EXTRA_TARGETS += docTarget

include(modeling/modeling.pri)

DISTFILES += .qmake.conf \
	sync.profile
