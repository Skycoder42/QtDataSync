TEMPLATE = subdirs

SUBDIRS += plain
qtHaveModule(KWallet): SUBDIRS += kwallet
unix:!android:!ios:system(pkg-config --exists libsecret-1): SUBDIRS += secretservice
win32: SUBDIRS += wincred
