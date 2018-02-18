#include "exchangedialog.h"
#include "ui_exchangedialog.h"
#include <QInputDialog>
#include <QMessageBox>

ExchangeDialog::ExchangeDialog(QtDataSync::AccountManager *manager, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ExchangeDialog),
	_manager(new QtDataSync::UserExchangeManager(manager, this))
{
	ui->setupUi(this);
	ui->portSpinBox->setValue(QtDataSync::UserExchangeManager::DataExchangePort);

	connect(_manager, &QtDataSync::UserExchangeManager::activeChanged,
			ui->activeCheckBox, &QCheckBox::setChecked);
	connect(_manager, &QtDataSync::UserExchangeManager::devicesChanged,
			this, &ExchangeDialog::devicesChanged);
	connect(_manager, &QtDataSync::UserExchangeManager::userDataReceived,
			this, &ExchangeDialog::userDataReceived);
	connect(_manager, &QtDataSync::UserExchangeManager::exchangeError,
			this, &ExchangeDialog::exchangeError);
}

ExchangeDialog::~ExchangeDialog()
{
	delete ui;
}

void ExchangeDialog::exec(QtDataSync::AccountManager *manager, QWidget *parent)
{
	ExchangeDialog dialog(manager, parent);
	dialog.QDialog::exec();
}

void ExchangeDialog::devicesChanged(QList<QtDataSync::UserInfo> devices)
{
	ui->treeWidget->clear();
	for(auto dev : devices) {
		auto item = new QTreeWidgetItem(ui->treeWidget);
		item->setText(0, dev.name());
		item->setText(1, QStringLiteral("%1:%2").arg(dev.address().toString()).arg(dev.port()));
		item->setData(1, Qt::UserRole + 1, QVariant::fromValue(dev));
	}
}

void ExchangeDialog::userDataReceived(const QtDataSync::UserInfo &userInfo, bool trusted)
{
	if(trusted) {
		auto ok = false;
		auto pw = QInputDialog::getText(this,
										tr("Import trusted data"),
										tr("Import data from %1. Enter the shared password:").arg(userInfo.name()),
										QLineEdit::Password,
										QString(),
										&ok);
		if(ok)
			_manager->importTrustedFrom(userInfo, pw, [this](bool ok, QString e) {
				if(ok)
					QMessageBox::information(this, tr("Import trusted data"), tr("Import succeeded!"));
				else
					QMessageBox::critical(this, tr("Import trusted data"), tr("Import failed with error:\n%1").arg(e));
			});
	} else {
		auto ok = QMessageBox::question(this,
										tr("Import untrusted data"),
										tr("Import data from %1?").arg(userInfo.name()));
		if(ok == QMessageBox::Yes)
			_manager->importFrom(userInfo, [this](bool ok, QString e) {
				if(ok)
					QMessageBox::information(this, tr("Import untrusted data"), tr("Import succeeded!"));
				else
					QMessageBox::critical(this, tr("Import untrusted data"), tr("Import failed with error:\n%1").arg(e));
			});
	}
}

void ExchangeDialog::exchangeError(const QString &errorString)
{
	QMessageBox::critical(this, tr("Exchange error"), errorString);
}

void ExchangeDialog::on_treeWidget_itemActivated(QTreeWidgetItem *item, int column)
{
	Q_UNUSED(column)
	if(item) {
		auto dev = item->data(1, Qt::UserRole + 1).value<QtDataSync::UserInfo>();
		auto pw = QInputDialog::getText(this,
										tr("Secure export data"),
										tr("Export data to %1. Enter a password or send untrusted:").arg(dev.name()),
										QLineEdit::Password);
		if(pw.isEmpty())
			_manager->exportTo(dev, false);
		else
			_manager->exportTrustedTo(dev, false, pw);
	}
}

void ExchangeDialog::on_activeCheckBox_clicked(bool checked)
{
	if(checked){
		if(_manager->startExchange(QHostAddress::Any, (quint16)ui->portSpinBox->value()))
			ui->portSpinBox->setValue(_manager->port());
		else
			ui->activeCheckBox->setChecked(false);
	} else
		_manager->stopExchange();
}
