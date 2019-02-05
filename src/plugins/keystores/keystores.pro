TEMPLATE = subdirs

SUBDIRS += plain
qtHaveModule(KWallet): SUBDIRS += kwallet
unix:!android:!ios:packagesExist(libsecret-1): SUBDIRS += secretservice
win32:!cross_compile: SUBDIRS += wincred
mac|ios: SUBDIRS += keychain
android:qtHaveModule(androidextras): SUBDIRS += android
