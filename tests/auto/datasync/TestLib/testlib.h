#ifndef TESTLIB_H
#define TESTLIB_H

#include <QtDataSync>

class TestLib
{
public:
	static QtDataSync::Setup &setup(QtDataSync::Setup &setup);

	static QTemporaryDir tDir;
};

#endif // TESTLIB_H
