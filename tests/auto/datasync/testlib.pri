QT *= datasync-private

LIB_PWD = $$shadowed($$PWD)

win32:!win32-g++:CONFIG(release, debug|release): LIBS += -L$$LIB_PWD/TestLib/release/ -lTestLib
else:win32:!win32-g++:CONFIG(debug, debug|release): LIBS += -L$$LIB_PWD/TestLib/debug/ -lTestLib
else: LIBS += -L$$LIB_PWD/TestLib/ -lTestLib

INCLUDEPATH += $$PWD/TestLib
DEPENDPATH += $$PWD/TestLib

#win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$LIB_PWD/TestLib/release/libTestLib.a
#else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$LIB_PWD/TestLib/debug/libTestLib.a
win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$LIB_PWD/TestLib/release/TestLib.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$LIB_PWD/TestLib/debug/TestLib.lib
else: PRE_TARGETDEPS += $$LIB_PWD/TestLib/libTestLib.a
