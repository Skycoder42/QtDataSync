#include "mainwidget.h"
#include "sampledata.h"
#include "setupdialog.h"
#include "ui_mainwidget.h"
#include <setup.h>

static QtMessageHandler prevHandler;
static void filterLogger(QtMsgType type, const QMessageLogContext &context, const QString &msg);

MainWidget::MainWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::MainWidget),
	store(nullptr)
{
	ui->setupUi(this);
	connect(ui->reloadButton, &QPushButton::clicked,
			this, &MainWidget::reload);

	prevHandler = qInstallMessageHandler(filterLogger);
	QMetaObject::invokeMethod(this, "setup", Qt::QueuedConnection);
}

MainWidget::~MainWidget()
{
	delete ui;
}

void MainWidget::on_addButton_clicked()
{
	auto data = new SampleData(this);
	data->id = ui->idSpinBox->value();
	data->title = ui->titleLineEdit->text();
	data->description = ui->detailsLineEdit->text();
	update(data);

	store->save<SampleData*>(data).onResult([this, data](){
		report(QtInfoMsg, QStringLiteral("Data with id %1 saved!").arg(data->id));
		data->deleteLater();
	}, [this, data](QException &exception) {
		report(QtCriticalMsg, exception.what());
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
			report(QtInfoMsg, QStringLiteral("Data with id %1 removed!").arg(id));
		}, [this](QException &exception) {
			report(QtCriticalMsg, exception.what());
		});
	}
}

void MainWidget::report(QtMsgType type, const QString &message)
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

void MainWidget::dataChanged(int metaTypeId, const QString &key, bool wasDeleted)
{
	if(wasDeleted) {
		auto item = items.take(key.toInt());
		if(item)
			delete item;
	} else {
		store->load(metaTypeId, key).toGeneric<SampleData*>().onResult([this, key](SampleData* data){
			report(QtInfoMsg, QStringLiteral("Data with id %1 changed").arg(key));
			update(data);
		}, [this](QException &exception) {
			report(QtCriticalMsg, QString::fromUtf8(exception.what()));
		});
	}
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
		report(QtInfoMsg, "All Data loaded from store!");
		foreach (auto d, data)
			update(d);
	}, [this](QException &exception) {
		report(QtCriticalMsg, QString::fromUtf8(exception.what()));
	});
}

void MainWidget::setup()
{
	if(SetupDialog::setup(this)) {
		store = new QtDataSync::AsyncDataStore(this);
		connect(store, &QtDataSync::AsyncDataStore::dataChanged,
				this, &MainWidget::dataChanged);
		reload();
	} else
		qApp->quit();
}

static void filterLogger(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
	if(QByteArray(context.category).startsWith("QtDataSync:")) {
		auto window = qApp->property("__mw").value<MainWidget*>();
		if(window) {
			auto rMsg = qFormatLogMessage(type, context, msg);
			window->report(type, rMsg);
			return;
		}
	}

	prevHandler(type, context, msg);
}
