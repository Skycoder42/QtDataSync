include(../tests.pri)

TARGET = tst_rothreadedbackend

SOURCES += \
		tst_rothreadedbackend.cpp

REPC_SOURCE += testclass.rep
REPC_REPLICA += $$REPC_SOURCE
