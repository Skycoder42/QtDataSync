#include "enginedatamodel_p.h"
using namespace QtDataSync;

EngineDataModel::EngineDataModel(QObject *parent) :
	QScxmlCppDataModel{parent}
{}

int EngineDataModel::callCnt(const QByteArray &name, bool clearCnt)
{
	const auto cnt = _calls[name];
	if (clearCnt)
		_calls[name] = 0;
	return cnt;
}

int EngineDataModel::totalCallCnt(bool clearCnt)
{
	auto sum = 0;
	for (auto val : qAsConst(_calls))
		sum += val;
	if (clearCnt)
		_calls.clear();
	return sum;
}

bool EngineDataModel::hasError() const
{
	return _hasError;
}
