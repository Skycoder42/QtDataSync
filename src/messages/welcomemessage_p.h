#ifndef QTDATASYNC_WELCOMEMESSAGE_P_H
#define QTDATASYNC_WELCOMEMESSAGE_P_H

#include <QtCore/QUuid>

#include "message_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT WelcomeMessage : public Message
{
	Q_GADGET

	Q_PROPERTY(bool hasChanges MEMBER hasChanges)
	Q_PROPERTY(quint32 keyIndex MEMBER keyIndex)
	Q_PROPERTY(QByteArray scheme MEMBER scheme)
	Q_PROPERTY(QByteArray key MEMBER key)
	Q_PROPERTY(QByteArray cmac MEMBER cmac)

public:
	WelcomeMessage(bool hasChanges = false);

	bool hasChanges;
	quint32 keyIndex;
	QByteArray scheme;
	QByteArray key;
	QByteArray cmac;

	bool hasKeyUpdate() const;
	QByteArray signatureData(const QUuid &deviceId) const;

protected:
	const QMetaObject *getMetaObject() const override;
};

}

Q_DECLARE_METATYPE(QtDataSync::WelcomeMessage)

#endif // QTDATASYNC_WELCOMEMESSAGE_P_H
