TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
	datasync \
    plugins

docTarget.target = doxygen
QMAKE_EXTRA_TARGETS += docTarget
