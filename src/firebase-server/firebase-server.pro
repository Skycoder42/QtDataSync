TEMPLATE = aux

FIREBASE_FILES += \
	firebase.json \
	firestore.indexes.json \
	firestore.rules

PKG_FILES += \
	functions/package.json \
	functions/tsconfig.json

TS_FILES += \
	functions/src/index.ts

defineTest(createCpComp) {
	name = $$1
	path = $$2
	in = $$3
	out = $$4

	$${name}.name = $$QMAKE_COPY_FILE ${QMAKE_FILE_IN}
	$${name}.input = $${in}
	$${name}.variable_out = $${out}
	$${name}.commands = $$QMAKE_CHK_DIR_EXISTS ${QMAKE_FILE_OUT_PATH} || $$QMAKE_MKDIR ${QMAKE_FILE_OUT_PATH} \
		$$escape_expand(\n\t)$$QMAKE_COPY_FILE ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
	$${name}.output = $${path}/${QMAKE_FILE_IN_BASE}${QMAKE_FILE_EXT}
	$${name}.CONFIG += target_predeps no_link
	QMAKE_EXTRA_COMPILERS += $${name}

	!export($${name}.name):return(false)
	!export($${name}.input):return(false)
	!export($${name}.variable_out):return(false)
	!export($${name}.commands):return(false)
	!export($${name}.output):return(false)
	!export($${name}.CONFIG):return(false)
	!export(QMAKE_EXTRA_COMPILERS):return(false)
}

createCpComp(fbcpc, pkg/, FIREBASE_FILES, MODULE_OUT_FILES)
createCpComp(fncpc, pkg/functions, PKG_FILES, MODULE_OUT_FILES)
createCpComp(tscpc, pkg/functions/src, TS_FILES, TS_BUILD_FILES)

npminstall.target = pkg/functions/node_modules
npminstall.depends += pkg/functions/package.json
npminstall.commands = cd pkg/functions && npm install
QMAKE_EXTRA_TARGETS += npminstall

tsc.name = tsc ${QMAKE_FILE_IN}
tsc.input = TS_BUILD_FILES
tsc.variable_out = JS_FILES
tsc.commands = cd pkg/functions && node_modules/.bin/tsc
tsc.output = pkg/functions/index.js
tsc.CONFIG += target_predeps combine
tsc.depends += pkg/functions/node_modules pkg/functions/tsconfig.json
QMAKE_EXTRA_COMPILERS += tsc

deploy.target = firestore-deploy
deploy.depends += pkg/functions/index.js
deploy.commands = cd pkg/functions && node_modules/.bin/firebase deploy --only functions
QMAKE_EXTRA_TARGETS += deploy

package.target = firebase-server.zip
package.depends += pkg/functions/index.js firebase-server
package.commands += cd pkg/functions && npm prune --production \
	$$escape_expand(\n\t)cd pkg && 7z a -r- ../firebase-server.zip \
		functions/node_modules \
		functions/*.js \
		functions/package*.json \
		*.* \
	$$escape_expand(\n\t)touch pkg/functions/package.json
QMAKE_EXTRA_TARGETS += package

firebase_server.files = $$shadowed(firebase-server.zip)
firebase_server.depends += firebase-server.zip
firebase_server.path = $$[QT_INSTALL_DATA]/datasync-server
firebase_server.CONFIG += no_check_exist
INSTALLS += firebase_server
