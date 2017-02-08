win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../../QtDataSync/release/ -lQt5DataSync
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../../QtDataSync/debug/ -lQt5DataSync
else:mac: LIBS += -F$$OUT_PWD/../../QtDataSync/ -framework Qt5DataSync
else:unix: LIBS += -L$$OUT_PWD/../../QtDataSync/ -lQt5DataSync

INCLUDEPATH += $$PWD/../QtDataSync
DEPENDPATH += $$PWD/../QtDataSync

HEADERS += \
	$$PWD/testdata.h

SOURCES += \
	$$PWD/testdata.cpp

INCLUDEPATH += $$PWD
