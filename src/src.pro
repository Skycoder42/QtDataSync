TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
	datasync \
    plugins \
    imports

docTarget.target = doxygen
QMAKE_EXTRA_TARGETS += docTarget
