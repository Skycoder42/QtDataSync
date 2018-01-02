#ifndef ACCOUNTDIALOG_H
#define ACCOUNTDIALOG_H

#include <QDialog>
#include <QAbstractButton>

#include <QtDataSync/accountmanager.h>

namespace Ui {
class AccountDialog;
}

class AccountDialog : public QDialog
{
	Q_OBJECT

public:
	explicit AccountDialog(QWidget *parent = nullptr);
	~AccountDialog();

	static void exec(QWidget *parent = nullptr);

private Q_SLOTS:
	void updateDevices(const QList<QtDataSync::DeviceInfo> &devices);
	void login(QtDataSync::LoginRequest * const request);

	void on_action_Remove_Device_triggered();

	void on_buttonBox_clicked(QAbstractButton *button);

private:
	Ui::AccountDialog *ui;
	QtDataSync::AccountManager *_manager;

	static QString printFingerprint(const QByteArray &fingerprint);
};

#endif // ACCOUNTDIALOG_H
