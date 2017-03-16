#include "setupdialog.h"
#include "ui_setupdialog.h"

#include <QSettings>
#include <wsauthenticator.h>

bool SetupDialog::setup(QWidget *parent)
{
	SetupDialog dialog(parent);

	QSettings settings;
	dialog.ui->storageDirComboBox->addItems(settings.value(QStringLiteral("localDirs"),
														   QStringList(dialog.ui->storageDirComboBox->currentText()))
											.toStringList());
	dialog.ui->remoteURLLineEdit->setText(settings.value(QStringLiteral("remoteUrl"),
														 dialog.ui->remoteURLLineEdit->text())
										  .toString());
	dialog.ui->regexSearchCheckBox->setChecked(settings.value(QStringLiteral("useRegex"),
															  dialog.ui->regexSearchCheckBox->isChecked())
											   .toBool());

	if(dialog.exec() == QDialog::Accepted) {
		QtDataSync::Setup()
				.setLocalDir(dialog.ui->storageDirComboBox->currentText())
				.setProperty("useRegex", dialog.ui->regexSearchCheckBox->isChecked())
				.create();

		auto auth = QtDataSync::Setup::authenticatorForSetup<QtDataSync::WsAuthenticator>(&dialog);
		auth->setRemoteUrl(dialog.ui->remoteURLLineEdit->text());
		auth->setValidateServerCertificate(false);
		auth->reconnect();

		QStringList items;
		for(auto i = 0; i < dialog.ui->storageDirComboBox->count(); i++)
			items.append(dialog.ui->storageDirComboBox->itemText(i));
		if(!items.contains(dialog.ui->storageDirComboBox->currentText()))
			items.append(dialog.ui->storageDirComboBox->currentText());
		settings.setValue(QStringLiteral("localDirs"), items);
		settings.setValue(QStringLiteral("remoteUrl"), dialog.ui->remoteURLLineEdit->text());
		settings.setValue(QStringLiteral("useRegex"), dialog.ui->regexSearchCheckBox->isChecked());

		return true;
	} else
		return false;
}

SetupDialog::SetupDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::SetupDialog)
{
	ui->setupUi(this);
}

SetupDialog::~SetupDialog()
{
	delete ui;
}
