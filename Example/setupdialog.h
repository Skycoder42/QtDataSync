#ifndef SETUPDIALOG_H
#define SETUPDIALOG_H

#include <QDialog>
#include <setup.h>

namespace Ui {
class SetupDialog;
}

class SetupDialog : public QDialog
{
	Q_OBJECT

public:
	static bool setup(QWidget *parent = nullptr);

private:
	Ui::SetupDialog *ui;

	explicit SetupDialog(QWidget *parent = nullptr);
	~SetupDialog();
};

#endif // SETUPDIALOG_H
