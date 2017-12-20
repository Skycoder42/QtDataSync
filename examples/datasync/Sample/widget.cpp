#include "widget.h"
#include "ui_widget.h"
#include "sampledata.h"
#include <QDebug>
#include <QMetaEnum>

#include "modeltest.h"

Widget::Widget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::Widget),
	_manager(new QtDataSync::SyncManager(this)),
	_model(new QtDataSync::DataStoreModel(this))
{
	ui->setupUi(this);

	connect(ui->syncButton, &QPushButton::clicked,
			_manager, &QtDataSync::SyncManager::synchronize);
	connect(ui->reconnectButton, &QPushButton::clicked,
			_manager, &QtDataSync::SyncManager::reconnect);
	connect(ui->syncCheckBox, &QCheckBox::clicked,
			_manager, &QtDataSync::SyncManager::setSyncEnabled);

	connect(_manager, &QtDataSync::SyncManager::syncEnabledChanged,
			ui->syncCheckBox, &QCheckBox::setChecked);
	connect(_manager, &QtDataSync::SyncManager::syncStateChanged,
			this, &Widget::updateState);

	ui->syncCheckBox->setChecked(_manager->isSyncEnabled());
	updateState(_manager->syncState());

	connect(_model, &QtDataSync::DataStoreModel::storeError, this, [](const QException &e){
		qCritical() << e.what();
	});

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

void Widget::updateState(QtDataSync::SyncManager::SyncState state)
{
	ui->stateLabel->setText(QStringLiteral("State: %1")
							.arg(QString::fromUtf8(QMetaEnum::fromType<QtDataSync::SyncManager::SyncState>().valueToKey(state))));
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
