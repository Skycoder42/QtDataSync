#ifndef QQMLACCOUNTMANAGER_H
#define QQMLACCOUNTMANAGER_H

#include <QtCore/QObject>

#include <QtQml/QJSValue>
#include <QtQml/QQmlParserStatus>

#include <QtDataSync/accountmanager.h>

namespace QtDataSync {

class QQmlAccountManager : public AccountManager, public QQmlParserStatus
{
	Q_OBJECT
	Q_DISABLE_COPY(QQmlAccountManager)
	Q_INTERFACES(QQmlParserStatus)

	Q_PROPERTY(QString setupName READ setupName WRITE setSetupName NOTIFY setupNameChanged)
	Q_PROPERTY(QRemoteObjectNode* node READ node WRITE setNode NOTIFY nodeChanged)
	Q_PROPERTY(bool valid READ valid NOTIFY validChanged)

public:
	explicit QQmlAccountManager(QObject *parent = nullptr);

	void classBegin() override;
	void componentComplete() override;

	QString setupName() const;
	QRemoteObjectNode* node() const;
	bool valid() const;

	Q_INVOKABLE bool isTrustedImport(const QJsonObject &importData) const;

	Q_INVOKABLE void exportAccount(bool includeServer, QJSValue completedFn, QJSValue errorFn = {});
	Q_INVOKABLE void exportAccountTrusted(bool includeServer, const QString &password, QJSValue completedFn, QJSValue errorFn = {});
	Q_INVOKABLE void importAccount(const QJsonObject &importData, QJSValue completedFn, bool keepData = false);
	Q_INVOKABLE void importAccountTrusted(const QJsonObject &importData, const QString &password, QJSValue completedFn, bool keepData = false);

public Q_SLOTS:
	void setSetupName(QString setupName);
	void setNode(QRemoteObjectNode* node);

Q_SIGNALS:
	void setupNameChanged(QString setupName);
	void nodeChanged(QRemoteObjectNode* node);
	void validChanged(bool valid);

private:
	QString _setupName;
	QRemoteObjectNode* _node;
	bool _valid;
};

}

#endif // QQMLACCOUNTMANAGER_H
