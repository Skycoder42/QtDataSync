#include "tabledatamodel_p.h"
using namespace QtDataSync;

TableDataModel::TableDataModel(QObject *parent) :
	QScxmlCppDataModel{parent}
{}

int TableDataModel::callCnt(const QByteArray &name, bool clearCnt)
{
	const auto cnt = _calls[name];
	if (clearCnt)
		_calls[name] = 0;
	return cnt;
}

int TableDataModel::totalCallCnt(bool clearCnt)
{
	auto sum = 0;
	for (auto val : qAsConst(_calls))
		sum += val;
	if (clearCnt)
		_calls.clear();
	return sum;
}
