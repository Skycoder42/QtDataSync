TEMPLATE = subdirs

SUBDIRS = datasync

docTarget.target = doxygen
QMAKE_EXTRA_TARGETS += docTarget
