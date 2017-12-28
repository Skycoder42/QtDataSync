TEMPLATE = subdirs

SUBDIRS += plain
qtHaveModule(KWallet): SUBDIRS += kwallet
unix:!android:system(pkg-config --exists libsecret-1): SUBDIRS += secretservice
