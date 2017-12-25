TEMPLATE = subdirs

SUBDIRS += plain
qtHaveModule(KWallet): SUBDIRS += kwallet
unix:system(pkg-config --exists libsecret-1): SUBDIRS += gsecret
