#ifndef QTDATASYNC_ERRORMESSAGE_P_H
#define QTDATASYNC_ERRORMESSAGE_P_H

#include <QtCore/QDebug>

#include "message_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT ErrorMessage : public Message
{
	Q_GADGET

	Q_PROPERTY(ErrorType type MEMBER type)
	Q_PROPERTY(QtDataSync::Utf8String message MEMBER message)
	Q_PROPERTY(bool canRecover MEMBER canRecover)

public:
	enum ErrorType {
		UnknownError = 0,
		IncompatibleVersionError = 1,
		UnexpectedMessageError = 2,
		ServerError = 3,
		ClientError = 4,
		AuthenticationError = 5,
		AccessError = 6,
		KeyIndexError = 7,
		KeyPendingError = 8,
		QuotaHitError = 9
	};
	Q_ENUM(ErrorType)

	ErrorMessage(ErrorType type = UnknownError,
				 const QString &message = {},
				 bool canRecover = false);

	ErrorType type;
	Utf8String message;
	bool canRecover;

protected:
	const QMetaObject *getMetaObject() const override;
	bool validate() override;
};

Q_DATASYNC_EXPORT QDebug operator<<(QDebug debug, const ErrorMessage &message);

}

Q_DECLARE_METATYPE(QtDataSync::ErrorMessage)

#endif // QTDATASYNC_ERRORMESSAGE_P_H
