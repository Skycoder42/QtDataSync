TEMPLATE = aux

QDEP_LUPDATE_INPUTS += $$PWD/../datasync
QDEP_LUPDATE_INPUTS += $$PWD/../datasyncandroid
QDEP_LUPDATE_INPUTS += $$PWD/../datasyncios
QDEP_LUPDATE_INPUTS += $$PWD/../imports
QDEP_LUPDATE_INPUTS += $$PWD/../java
QDEP_LUPDATE_INPUTS += $$PWD/../messages
QDEP_LUPDATE_INPUTS += $$PWD/../plugins

TRANSLATIONS += \
	qtdatasync_de.ts \
	qtdatasync_template.ts

CONFIG += lrelease
QM_FILES_INSTALL_PATH = $$[QT_INSTALL_TRANSLATIONS]

QDEP_DEPENDS +=

!load(qdep):error("Failed to load qdep feature! Run 'qdep prfgen --qmake $$QMAKE_QMAKE' to create it.")

#replace template qm by ts
QM_FILES -= $$__qdep_lrelease_real_dir/qtdatasync_template.qm
QM_FILES += qtdatasync_template.ts

HEADERS =
SOURCES =
GENERATED_SOURCES =
OBJECTIVE_SOURCES =
RESOURCES =
