#include "testlib.h"
using namespace QtDataSync;

QTemporaryDir TestLib::tDir;

Setup &TestLib::setup(Setup &setup)
{
	return setup.setLocalDir(tDir.path());
}
