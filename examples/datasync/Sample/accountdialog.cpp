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

	if(_manager->replica()->isInitialized())
		_manager->listDevices();
	else {
		connect(_manager->replica(), &QRemoteObjectReplica::initialized,
				_manager, &QtDataSync::AccountManager::listDevices);
	}
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
	ui->treeWidget->clear();
	foreach(auto device, devices) {
		auto item = new QTreeWidgetItem(ui->treeWidget);
		item->setText(0, device.name());
		item->setText(1, printFingerprint(device.fingerprint()));
		item->setData(0, Qt::UserRole + 1, device.deviceId());
	}
}

void AccountDialog::login(QtDataSync::LoginRequest * const request)
{
	Q_UNIMPLEMENTED();
}

QString AccountDialog::printFingerprint(const QByteArray &fingerprint)
{
	QByteArrayList res;
	foreach(char c, fingerprint)
		res.append(QByteArray(1, c).toHex());
	return QString::fromUtf8(res.join(':'));
}

void AccountDialog::on_action_Remove_Device_triggered()
{
	auto item = ui->treeWidget->currentItem();
	if(item) {
		auto id = item->data(0, Qt::UserRole + 1).toUuid();
		if(!id.isNull())
			_manager->removeDevice(id);
	}
}

void AccountDialog::on_buttonBox_clicked(QAbstractButton *button)
{
	if(ui->buttonBox->standardButton(button) == QDialogButtonBox::Reset)
		_manager->resetAccount(true);
	else if(ui->buttonBox->standardButton(button) == QDialogButtonBox::RestoreDefaults)
		_manager->resetAccount(false);
}
