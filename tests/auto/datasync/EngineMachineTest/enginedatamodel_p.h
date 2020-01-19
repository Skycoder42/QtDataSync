#ifndef ENGINEDATAMODEL_H
#define ENGINEDATAMODEL_H

#include <QtScxml/QScxmlCppDataModel>

#define DECL_SCRIPT(name) inline void name() { \
	++_calls[QByteArrayLiteral(#name)]; \
}

namespace QtDataSync {

class EngineDataModel : public QScxmlCppDataModel
{
	Q_OBJECT
	Q_SCXML_DATAMODEL

public:
	enum class StopEvent {
		Stop,
		LogOut,
		DeleteAcc
	};
	Q_ENUM(StopEvent)

	using ScriptFn = void (EngineDataModel::*)();

	explicit EngineDataModel(QObject *parent = nullptr);

	int callCnt(const QByteArray &name, bool clearCnt = true);
	int totalCallCnt(bool clearCnt = true);

public /*scripts*/:
	bool hasError() const;

	DECL_SCRIPT(signIn)
	DECL_SCRIPT(removeUser)
	DECL_SCRIPT(cancelRmUser)
	DECL_SCRIPT(emitError)

/*Q_SIGNALS:*/
	DECL_SCRIPT(startTableSync)
	DECL_SCRIPT(stopTableSync)

public:
	bool _hasError = false;
	StopEvent _stopEv = StopEvent::Stop;

private:
	QHash<QByteArray, int> _calls;
};

}

#define COMPARE_CALLED(model, name, cnt) QCOMPARE((model)->callCnt(QByteArrayLiteral(#name)), (cnt))
#define VERIFY_EMPTY(model) QCOMPARE((model)->totalCallCnt(), 0)

#endif // ENGINEDATAMODEL_H
