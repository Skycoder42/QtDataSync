#include "testresolver.h"

TestResolver::TestResolver(QObject *parent) :
	GenericConflictResolver(parent)
{}

TestData TestResolver::resolveConflict(TestData data1, TestData data2, QObject *parent) const
{
	Q_ASSERT(parent);
	Q_ASSERT_X(data1.id == data2.id, Q_FUNC_INFO, "resolveConflict called on different datasets");

	TestData resData(data1.id);
	if(data1.text < data2.text)
		resData.text = data1.text + QStringLiteral("+conflict");
	else
		resData.text = data2.text + QStringLiteral("+conflict");
	return resData;
}
