TEMPLATE = subdirs

SUBDIRS += plain
qtHaveModule(KWallet): SUBDIRS += kwallet
unix:!android:!ios:system(pkg-config --exists libsecret-1): SUBDIRS += secretservice
win32:!winrt: SUBDIRS += wincred
mac|ios: SUBDIRS += keychain
android:qtHaveModule(androidextras): SUBDIRS += android

prepareRecursiveTarget(lrelease)
QMAKE_EXTRA_TARGETS += lrelease
