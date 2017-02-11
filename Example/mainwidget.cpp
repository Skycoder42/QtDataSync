#include "mainwidget.h"
#include "sampledata.h"
#include "setupdialog.h"
#include "ui_mainwidget.h"
#include <setup.h>

MainWidget::MainWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::MainWidget),
	store(nullptr)
{
	ui->setupUi(this);
	connect(ui->reloadButton, &QPushButton::clicked,
			this, &MainWidget::reload);
}

MainWidget::~MainWidget()
{
	delete ui;
}

void MainWidget::showEvent(QShowEvent *event)
{
	QWidget::showEvent(event);
	QMetaObject::invokeMethod(this, "setup", Qt::QueuedConnection);
}

void MainWidget::on_addButton_clicked()
{
	auto data = new SampleData(this);
	data->id = ui->idSpinBox->value();
	data->title = ui->titleLineEdit->text();
	data->description = ui->detailsLineEdit->text();
	update(data);

	store->save<SampleData*>(data).onResult([this, data](){
		report(true, QStringLiteral("Data with id %1 saved!").arg(data->id));
		data->deleteLater();
	}, [this, data](QException &exception) {
		report(false, exception.what());
		data->deleteLater();
	});
}

void MainWidget::on_deleteButton_clicked()
{
	auto item = ui->dataTreeWidget->currentItem();
	if(item) {
		auto id = items.key(item);
		items.remove(id);
		delete item;
		store->remove<SampleData*>(QString::number(id)).onResult([this, id](){
			report(true, QStringLiteral("Data with id %1 removed!").arg(id));
		}, [this](QException &exception) {
			report(false, exception.what());
		});
	}
}

void MainWidget::report(bool success, const QString &message)
{
	auto icon = style()->standardPixmap(success ?
											QStyle::SP_MessageBoxInformation :
											QStyle::SP_MessageBoxCritical);
	new QListWidgetItem(icon, message, ui->reportListWidget);
}

void MainWidget::update(SampleData *data)
{
	auto item = items.value(data->id);
	if(!item) {
		item = new QTreeWidgetItem(ui->dataTreeWidget);
		item->setText(0, QString::number(data->id));
		items.insert(data->id, item);
	}

	item->setText(1, data->title);
	item->setText(2, data->description);
}

void MainWidget::reload()
{
	items.clear();
	ui->dataTreeWidget->clear();

	store->loadAll<SampleData*>().onResult([this](QList<SampleData*> data){
		report(true, "All Data loaded from store!");
		foreach (auto d, data)
			update(d);
	}, [this](QException &exception) {
		report(false, QString::fromLatin1(exception.what()));
	});
}

void MainWidget::setup()
{
	if(SetupDialog::setup(this)) {
		store = new QtDataSync::AsyncDataStore(this);
		reload();
	} else
		qApp->quit();
}
