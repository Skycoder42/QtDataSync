#ifndef EXCHANGEDIALOG_H
#define EXCHANGEDIALOG_H

#include <QDialog>
#include <QTreeWidgetItem>
#include <QtDataSync/accountmanager.h>
#include <QtDataSync/userexchangemanager.h>

namespace Ui {
class ExchangeDialog;
}

class ExchangeDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ExchangeDialog(QtDataSync::AccountManager *manager, QWidget *parent = nullptr);
	~ExchangeDialog();

	static void exec(QtDataSync::AccountManager *manager, QWidget *parent = nullptr);

private slots:
	void devicesChanged(QList<QtDataSync::UserInfo> devices);
	void userDataReceived(const QtDataSync::UserInfo &userInfo, bool trusted);
	void exchangeError(const QString &errorString);

	void on_treeWidget_itemActivated(QTreeWidgetItem *item, int column);
	void on_activeCheckBox_clicked(bool checked);

private:
	Ui::ExchangeDialog *ui;
	QtDataSync::UserExchangeManager *_manager;
};

#endif // EXCHANGEDIALOG_H
