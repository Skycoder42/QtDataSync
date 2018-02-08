TEMPLATE = subdirs

SUBDIRS += plain
qtHaveModule(KWallet): SUBDIRS += kwallet
unix:!android:!ios:system(pkg-config --exists libsecret-1): SUBDIRS += secretservice
win32:!winrt: SUBDIRS += wincred
mac|ios: SUBDIRS += keychain
android: SUBDIRS += android

plain.CONFIG += no_lrelease_target
kwallet.CONFIG += no_lrelease_target
secretservice.CONFIG += no_lrelease_target
wincred.CONFIG += no_lrelease_target
keychain.CONFIG += no_lrelease_target
android.CONFIG += no_lrelease_target

prepareRecursiveTarget(lrelease)
QMAKE_EXTRA_TARGETS += lrelease
