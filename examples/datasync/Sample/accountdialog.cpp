#include "accountdialog.h"
#include "ui_accountdialog.h"

#include <QRemoteObjectReplica>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>

#include "exchangedialog.h"

AccountDialog::AccountDialog(const QString &setup, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::AccountDialog),
	_manager(new QtDataSync::AccountManager(setup, this))
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
			this, &AccountDialog::login,
			Qt::DirectConnection); //must be direct

	connect(ui->deviceNameLineEdit, &QLineEdit::editingFinished,
			_manager, [this]() {
		_manager->setDeviceName(ui->deviceNameLineEdit->text());
	});
	connect(ui->action_Reset_Devicename, &QAction::triggered,
			_manager, &QtDataSync::AccountManager::resetDeviceName);
	connect(ui->updateKeyButton, &QPushButton::clicked,
			_manager, &QtDataSync::AccountManager::updateExchangeKey);
	connect(_manager, &QtDataSync::AccountManager::importAccepted,
			this, &AccountDialog::importDone);
	connect(_manager, &QtDataSync::AccountManager::lastErrorChanged,
			this, &AccountDialog::engineError);
	connect(_manager, &QtDataSync::AccountManager::accountAccessGranted,
			_manager, &QtDataSync::AccountManager::listDevices);

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

void AccountDialog::exec(const QString &setup, QWidget *parent)
{
	AccountDialog dialog(setup, parent);
	dialog.QDialog::exec();
}

void AccountDialog::updateDevices(const QList<QtDataSync::DeviceInfo> &devices)
{
	ui->treeWidget->clear();
	for(auto device : devices) {
		auto item = new QTreeWidgetItem(ui->treeWidget);
		item->setText(0, device.name());
		item->setText(1, printFingerprint(device.fingerprint()));
		item->setData(0, Qt::UserRole + 1, device.deviceId());
	}
}

void AccountDialog::importDone()
{
	_manager->listDevices();
	QMessageBox::information(this, tr("Account Import"), tr("Import was excepted by partner. Login completed"));
}

void AccountDialog::engineError(const QString &error)
{
	if(!error.isEmpty())
		QMessageBox::critical(this, tr("Engine Error"), error);
}

void AccountDialog::login(QtDataSync::LoginRequest request)
{
	if(request.handled())
		return;
	if(QMessageBox::question(this,
							 tr("Login requested"),
							 tr("A device wants to log into your account:<p>"
								"Name: %1<br/>"
								"Fingerprint: %2</p>"
								"Do you want accept the request?")
							 .arg(request.device().name())
							 .arg(printFingerprint(request.device().fingerprint())))
	   == QMessageBox::Yes)
		request.accept();
	else
		request.reject();
}

QString AccountDialog::printFingerprint(const QByteArray &fingerprint)
{
	QByteArrayList res;
	for(char c : fingerprint)
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

void AccountDialog::on_pushButton_clicked()
{
	bool ok = false;
	auto pw = QInputDialog::getText(this,
									tr("Account Export"),
									tr("Enter a password or leave it empty:"),
									QLineEdit::Password,
									QString(),
									&ok);
	if(!ok)
		return;

	auto path = QFileDialog::getSaveFileName(this, tr("Account Export"), QStringLiteral("/tmp/"));
	if(!path.isEmpty()) {
		auto fn = [this, path](QByteArray d) {
			QFile f(path);
			f.open(QIODevice::WriteOnly);
			f.write(d);
			f.close();
			QMessageBox::information(this, tr("Account Export"), tr("Account export done!"));
		};
		auto eFn = [this](QString error) {
			QMessageBox::critical(this, tr("Account Export failed!"), error);
		};
		if(pw.isNull())
			_manager->exportAccount(true, fn, eFn);
		else
			_manager->exportAccountTrusted(true, pw, fn, eFn);
	}
}

void AccountDialog::on_pushButton_2_clicked()
{
	auto path = QFileDialog::getOpenFileName(this, tr("Account Import"), QStringLiteral("/tmp/"));
	if(!path.isEmpty()) {
		QFile f(path);
		f.open(QIODevice::ReadOnly);
		auto data = f.readAll();
		f.close();

		auto fn = [this](bool ok, QString error) {
			if(ok)
				QMessageBox::information(this, tr("Account Import"), tr("Account import successful!"));
			else
				QMessageBox::critical(this, tr("Account Import failed!"), error);
		};

		if(QtDataSync::AccountManager::isTrustedImport(data)) {
			bool ok = false;
			auto pw = QInputDialog::getText(this,
											tr("Account Import"),
											tr("Enter the password for the imported data:"),
											QLineEdit::Password,
											QString(),
											&ok);
			if(!ok)
				return;
			_manager->importAccountTrusted(data, pw, fn);
		} else
			_manager->importAccount(data, fn);
	}
}

void AccountDialog::on_exchangeButton_clicked()
{
	ExchangeDialog::exec(_manager, this);
}
