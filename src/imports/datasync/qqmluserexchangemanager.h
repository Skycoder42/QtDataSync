#ifndef QQMLUSEREXCHANGEMANAGER_H
#define QQMLUSEREXCHANGEMANAGER_H

#include <QtCore/QObject>

#include <QtQml/QQmlParserStatus>

#include <QtDataSync/userexchangemanager.h>

#include "qqmlaccountmanager.h"

namespace QtDataSync {

class QQmlUserExchangeManager : public UserExchangeManager, public QQmlParserStatus
{
	Q_OBJECT
	Q_DISABLE_COPY(QQmlUserExchangeManager)
	Q_INTERFACES(QQmlParserStatus)

	Q_PROPERTY(quint16 DataExchangePort READ DataExchangePort CONSTANT)
	Q_PROPERTY(QString setupName READ setupName WRITE setSetupName NOTIFY setupNameChanged)
	Q_PROPERTY(QQmlAccountManager* manager READ manager WRITE setManager NOTIFY managerChanged)
	Q_PROPERTY(bool valid READ valid NOTIFY validChanged)

public:
	explicit QQmlUserExchangeManager(QObject *parent = nullptr);

	void classBegin() override;
	void componentComplete() override;

	quint16 DataExchangePort() const;

	QString setupName() const;
	QQmlAccountManager* manager() const;
	bool valid() const;

	Q_INVOKABLE void importFrom(const QtDataSync::UserInfo &userInfo,
								QJSValue completedFn,
								bool keepData = false);
	Q_INVOKABLE void importTrustedFrom(const QtDataSync::UserInfo &userInfo,
									   const QString &password,
									   QJSValue completedFn,
									   bool keepData = false);

public Q_SLOTS:
	void setSetupName(QString setupName);
	void setManager(QQmlAccountManager* manager);

Q_SIGNALS:
	void setupNameChanged(QString setupName);
	void managerChanged(QQmlAccountManager* manager);
	void validChanged(bool valid);

private:
	QString _setupName;
	QQmlAccountManager* _manager;
};

}

#endif // QQMLUSEREXCHANGEMANAGER_H
