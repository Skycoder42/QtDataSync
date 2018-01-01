#include "accountdialog.h"
#include "ui_accountdialog.h"

#include <QRemoteObjectReplica>

AccountDialog::AccountDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::AccountDialog),
	_manager(new QtDataSync::AccountManager(this))
{
	ui->setupUi(this);
	ui->treeWidget->addAction(ui->action_Remove_Device);
	ui->deleteButton->setDefaultAction(ui->action_Remove_Device);
	ui->deviceNameLineEdit->addAction(ui->action_Reset_Devicename, QLineEdit::TrailingPosition);

	connect(_manager, &QtDataSync::AccountManager::deviceNameChanged,
			ui->deviceNameLineEdit, &QLineEdit::setText);
	connect(_manager, &QtDataSync::AccountManager::deviceFingerprintChanged,
			ui->fingerprintLineEdit, [this](QByteArray f) {
		ui->fingerprintLineEdit->setText(printFingerprint(f));
	});
	connect(_manager, &QtDataSync::AccountManager::accountDevices,
			this, &AccountDialog::updateDevices);
	connect(_manager, &QtDataSync::AccountManager::loginRequested,
			this, &AccountDialog::login);

	connect(ui->deviceNameLineEdit, &QLineEdit::editingFinished,
			_manager, [this]() {
		_manager->setDeviceName(ui->deviceNameLineEdit->text());
	});
	connect(ui->action_Reset_Devicename, &QAction::triggered,
			_manager, &QtDataSync::AccountManager::resetDeviceName);

	ui->deviceNameLineEdit->setText(_manager->deviceName());
	ui->fingerprintLineEdit->setText(printFingerprint(_manager->deviceFingerprint()));
}

AccountDialog::~AccountDialog()
{
	delete ui;
}

void AccountDialog::exec(QWidget *parent)
{
	AccountDialog dialog(parent);
	dialog.QDialog::exec();
}

void AccountDialog::updateDevices(const QList<QtDataSync::DeviceInfo> &devices)
{

}

void AccountDialog::login(QtDataSync::LoginRequest * const request)
{

}

QString AccountDialog::printFingerprint(const QByteArray &fingerprint)
{
	QByteArrayList res;
	foreach(char c, fingerprint)
		res.append(QByteArray(1, c).toHex());
	return QString::fromUtf8(res.join(':'));
}
