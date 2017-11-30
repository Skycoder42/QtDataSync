#include "widget.h"
#include "ui_widget.h"
#include "sampledata.h"
#include <QDebug>

#include "modeltest.h"

static QtMessageHandler prevHandler;
static void filterLogger(QtMsgType type, const QMessageLogContext &context, const QString &msg);

Widget::Widget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::Widget),
	_model(new QtDataSync::DataStoreModel(this))
{
	ui->setupUi(this);

	prevHandler = qInstallMessageHandler(filterLogger);

	new ModelTest(_model, this);
	_model->setTypeId<SampleData>();
	ui->dataTreeView->setModel(_model);
	connect(ui->dataTreeView->selectionModel(), &QItemSelectionModel::currentChanged,
			this, &Widget::selectionChange);
}

Widget::~Widget()
{
	delete ui;
}

void Widget::report(QtMsgType type, const QString &message)
{
	QIcon icon;
	switch (type) {
	case QtDebugMsg:
	case QtInfoMsg:
		icon = style()->standardPixmap(QStyle::SP_MessageBoxInformation);
		break;
	case QtWarningMsg:
		icon = style()->standardPixmap(QStyle::SP_MessageBoxWarning);
		break;
	case QtCriticalMsg:
	case QtFatalMsg:
		icon = style()->standardPixmap(QStyle::SP_MessageBoxCritical);
		break;
	default:
		Q_UNREACHABLE();
		break;
	}
	new QListWidgetItem(icon, message, ui->reportListWidget);
}

void Widget::selectionChange(const QModelIndex &index)
{
	try {
		auto data = _model->loadObject(index).value<SampleData>();
		qDebug() << data;
		ui->idSpinBox->setValue(data.id);
		ui->titleLineEdit->setText(data.title);
		ui->detailsLineEdit->setText(data.description);
	} catch(QtDataSync::NoDataException &e) {
		qDebug() << e.what();
		ui->idSpinBox->clear();
		ui->titleLineEdit->clear();
		ui->detailsLineEdit->clear();
	} catch(QException &e) {
		report(QtCriticalMsg, QString::fromUtf8(e.what()));
	}
}

void Widget::on_addButton_clicked()
{
	SampleData data;
	data.id = ui->idSpinBox->value();
	data.title = ui->titleLineEdit->text();
	data.description = ui->detailsLineEdit->text();
	_model->store()->save(data);
}

void Widget::on_deleteButton_clicked()
{
	_model->store()->remove<SampleData>(ui->idSpinBox->value());
}

static void filterLogger(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
	if(QByteArray(context.category).startsWith("qtdatasync.")) {
		auto window = qApp->property("__mw").value<Widget*>();
		if(window) {
			auto rMsg = qFormatLogMessage(type, context, msg);
			window->report(type, rMsg);
			return;
		}
	}

	prevHandler(type, context, msg);
}
