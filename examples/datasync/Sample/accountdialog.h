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
	explicit AccountDialog(const QString &setup, QWidget *parent = nullptr);
	~AccountDialog();

	static void exec(const QString &setup, QWidget *parent = nullptr);

private Q_SLOTS:
	void updateDevices(const QList<QtDataSync::DeviceInfo> &devices);
	void login(QtDataSync::LoginRequest request);
	void importDone();
	void engineError(const QString &error);

	void on_action_Remove_Device_triggered();
	void on_buttonBox_clicked(QAbstractButton *button);
	void on_pushButton_clicked();
	void on_pushButton_2_clicked();

	void on_exchangeButton_clicked();

private:
	Ui::AccountDialog *ui;
	QtDataSync::AccountManager *_manager;

	static QString printFingerprint(const QByteArray &fingerprint);
};

#endif // ACCOUNTDIALOG_H
