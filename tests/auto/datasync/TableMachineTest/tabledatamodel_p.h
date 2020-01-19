#ifndef TABLEDATAMODEL_H
#define TABLEDATAMODEL_H

#include <QtCore/QDebug>
#include <QtScxml/QScxmlCppDataModel>

#define DECL_SCRIPT(name) inline void name() { \
	++_calls[QByteArrayLiteral(#name)]; \
}

namespace QtDataSync {

class TableDataModel : public QScxmlCppDataModel
{
	Q_OBJECT
	Q_SCXML_DATAMODEL

public:
	using ScriptFn = void (TableDataModel::*)();

	explicit TableDataModel(QObject *parent = nullptr);

	int callCnt(const QByteArray &name, bool clearCnt = true);
	int totalCallCnt(bool clearCnt = true);

public /*scripts*/:
	DECL_SCRIPT(downloadChanges)
	DECL_SCRIPT(processDownload)
	DECL_SCRIPT(uploadData)
	DECL_SCRIPT(initSync)
	DECL_SCRIPT(initLiveSync)
	DECL_SCRIPT(scheduleLsRestart)
	DECL_SCRIPT(clearLsRestart)
	DECL_SCRIPT(cancelPassiveSync)
	DECL_SCRIPT(cancelLiveSync)
	DECL_SCRIPT(cancelAll)
	DECL_SCRIPT(delTable)
	DECL_SCRIPT(emitError)
	DECL_SCRIPT(emitStop)

public /*members*/:
	bool _dlReady = true;
	bool _procReady = true;
	bool _delTable = false;

	QHash<QByteArray, int> _calls;
};

}

#define COMPARE_CALLED(model, name, cnt) QCOMPARE((model)->callCnt(QByteArrayLiteral(#name)), (cnt))
#define VERIFY_EMPTY(model) QCOMPARE((model)->totalCallCnt(), 0)

#endif // TABLEDATAMODEL_H
