TEMPLATE = subdirs

CONFIG += ordered
SUBDIRS += \
	QtDataSync \
	Tests

!android: SUBDIRS += QtDataSyncServer \
	Example
