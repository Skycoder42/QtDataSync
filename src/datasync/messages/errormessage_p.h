#ifndef ERRORMESSAGE_P_H
#define ERRORMESSAGE_P_H

#include <QtCore/QDebug>

#include "message_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT ErrorMessage
{
	Q_GADGET

	Q_PROPERTY(ErrorType type MEMBER type)
	Q_PROPERTY(bool canRecover MEMBER canRecover)
	Q_PROPERTY(QString message MEMBER message)

public:
	enum ErrorType {
		UnknownError = 0,
		IncompatibleVersionError
	};
	Q_ENUM(ErrorType)

	ErrorMessage(ErrorType type = UnknownError,
				 bool canRecover = false,
				 const QString &message = {});

	ErrorType type;
	bool canRecover;
	QString message;
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const ErrorMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, ErrorMessage &message);
Q_DATASYNC_EXPORT QDebug operator<<(QDebug debug, const ErrorMessage &message);

}

Q_DECLARE_METATYPE(QtDataSync::ErrorMessage)

#endif // ERRORMESSAGE_P_H
