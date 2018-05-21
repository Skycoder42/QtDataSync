INCLUDEPATH += $$PWD

MSG_DIR = $$shadowed($$dirname(_QMAKE_CONF_))/lib

win32:CONFIG(release, debug|release): LIBS_PRIVATE += -L$$MSG_DIR/ -lqtdatasync-messages
else:win32:CONFIG(debug, debug|release): LIBS_PRIVATE += -L$$MSG_DIR/ -lqtdatasync-messagesd
else:darwin:CONFIG(debug, debug|release): LIBS_PRIVATE += -L$$MSG_DIR/ -lqtdatasync-messages_debug
else: LIBS_PRIVATE += -L$$MSG_DIR/ -lqtdatasync-messages

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$MSG_DIR/libqtdatasync-messages.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$MSG_DIR/libqtdatasync-messagesd.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$MSG_DIR/qtdatasync-messages.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$MSG_DIR/qtdatasync-messagesd.lib
else:darwin:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$MSG_DIR/libqtdatasync-messages_debug.a
else: PRE_TARGETDEPS += $$MSG_DIR/libqtdatasync-messages.a

include($$PWD/../3rdparty/cryptopp/cryptopp.pri)
