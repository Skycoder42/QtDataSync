#include "widget.h"
#include "ui_widget.h"
#include "sampledata.h"
#include <QDebug>

#include "modeltest.h"

Widget::Widget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::Widget),
	_model(new QtDataSync::DataStoreModel(this))
{
	ui->setupUi(this);

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
		qCritical() << e.what();
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

void Widget::on_clearButton_clicked()
{
	_model->store()->clear<SampleData>();
}
