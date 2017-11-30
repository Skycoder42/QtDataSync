#include "sampledata.h"

QDebug operator<<(QDebug debug, const SampleData &data)
{
	QDebugStateSaver saver(debug);
	debug.nospace().noquote() << "SampleData{"
							  << "id: " << data.id << ", "
							  << "title: " << data.title << ", "
							  << "description: " << data.description
							  << '}';
	return debug;
}
