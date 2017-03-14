TEMPLATE = subdirs
SUBDIRS = qdatasyncserver

qdatasyncserver.CONFIG = host_build

docTarget.target = doxygen
QMAKE_EXTRA_TARGETS += docTarget 
