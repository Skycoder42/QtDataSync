#include "mainwidget.h"
#include "sampledata.h"
#include "setupdialog.h"
#include "ui_mainwidget.h"
#include <QInputDialog>
#include <cachingdatastore.h>
#include <setup.h>
#include <wsauthenticator.h>

static QtMessageHandler prevHandler;
static void filterLogger(QtMsgType type, const QMessageLogContext &context, const QString &msg);

MainWidget::MainWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::MainWidget),
	store(nullptr),
	sync(nullptr)
{
	ui->setupUi(this);

	prevHandler = qInstallMessageHandler(filterLogger);
	QMetaObject::invokeMethod(this, "setup", Qt::QueuedConnection);
}

MainWidget::~MainWidget()
{
	delete ui;
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

void MainWidget::dataResetted()
{
	items.clear();
	ui->dataTreeWidget->clear();
	report(QtInfoMsg, QStringLiteral("Data resetted"));
}

void MainWidget::updateProgress(int count)
{
	if(count == 0) {
		ui->progressBar->setValue(1);
		ui->progressBar->setMaximum(1);
	} else {
		auto max = qMax(count, ui->progressBar->maximum());
		auto value = max - count;
		ui->progressBar->setMaximum(max);
		ui->progressBar->setValue(value);
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

void MainWidget::setup()
{
	if(SetupDialog::setup(this)) {
		store = new QtDataSync::AsyncDataStore(this);
		connect(store, &QtDataSync::AsyncDataStore::dataChanged,
				this, &MainWidget::dataChanged);
		connect(store, &QtDataSync::AsyncDataStore::dataResetted,
				this, &MainWidget::dataResetted);

		sync = new QtDataSync::SyncController(this);
		connect(sync, &QtDataSync::SyncController::syncStateChanged,
				this, [](QtDataSync::SyncController::SyncState state) {
			qDebug() << state;
		});
		connect(sync, &QtDataSync::SyncController::syncOperationsChanged,
				this, &MainWidget::updateProgress);
		connect(sync, &QtDataSync::SyncController::authenticationErrorChanged,
				this, [](QString error) {
			if(error.isEmpty())
				qDebug() << "auth error cleared";
			else
				qDebug() << "auth error:" << error;
		});
		qDebug() << sync->syncState();
		if(!sync->authenticationError().isEmpty())
			qDebug() << "auth error:" << sync->authenticationError();
		connect(ui->reloadButton, &QPushButton::clicked,
				sync, &QtDataSync::SyncController::triggerSync);
		connect(ui->resyncButton, &QPushButton::clicked,
				sync, &QtDataSync::SyncController::triggerResync);

		store->loadAll<SampleData*>().onResult([this](QList<SampleData*> data){
			report(QtInfoMsg, "All Data loaded from store!");
			foreach (auto d, data)
				update(d);
		}, [this](QException &exception) {
			report(QtCriticalMsg, QString::fromUtf8(exception.what()));
		});

		//caching test
		QtDataSync::CachingDataStore<SampleData*, int> cacheStore(nullptr, true);
		qDebug() << "caching store test:" << cacheStore.keys();
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

void MainWidget::on_addButton_clicked()
{
	auto data = new SampleData(this);
	data->id = ui->idSpinBox->value();
	data->title = ui->titleLineEdit->text();
	data->description = ui->detailsLineEdit->text();

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
		store->remove<SampleData*>(QString::number(id)).onResult([this, id](bool success){
			if(success)
				report(QtInfoMsg, QStringLiteral("Data with id %1 removed!").arg(id));
			else
				report(QtInfoMsg, QStringLiteral("No entry with id %1 found!").arg(id));
		}, [this](QException &exception) {
			report(QtCriticalMsg, exception.what());
		});
	}
}

void MainWidget::on_changeUserButton_clicked()
{
	auto auth = QtDataSync::Setup::authenticatorForSetup<QtDataSync::WsAuthenticator>(this);
	auto user = auth->userIdentity();
	auto ok = false;
	auto str = QInputDialog::getText(this,
									 QStringLiteral("Change User"),
									 QStringLiteral("Enter the new ID or leave emtpy to generate one:"),
									 QLineEdit::Normal,
									 QString::fromUtf8(user),
									 &ok);
	if(ok) {
		user = str.toUtf8();
		QtDataSync::GenericTask<void> task;
		if(user.isEmpty())
			task = auth->resetUserIdentity();
		else
			task = auth->setUserIdentity(user);
		task.onResult([=](){
			qDebug() << "Changed userId to" << auth->userIdentity();
			auth->deleteLater();
		});
	}
}

void MainWidget::on_dataTreeWidget_itemSelectionChanged()
{
	auto item = ui->dataTreeWidget->currentItem();
	if(item) {
		ui->idSpinBox->setValue(item->text(0).toInt());
		ui->titleLineEdit->setText(item->text(1));
		ui->detailsLineEdit->setText(item->text(2));
	} else {
		ui->idSpinBox->clear();
		ui->titleLineEdit->clear();
		ui->detailsLineEdit->clear();
	}
}

void MainWidget::on_searchEdit_returnPressed()
{
	items.clear();
	ui->dataTreeWidget->clear();

	auto query = ui->searchEdit->text();
	if(query.isEmpty()) {
		store->loadAll<SampleData*>().onResult([this](QList<SampleData*> data){
			report(QtInfoMsg, "All Data loaded from store!");
			foreach (auto d, data)
				update(d);
		}, [this](QException &exception) {
			report(QtCriticalMsg, QString::fromUtf8(exception.what()));
		});
	} else {
		store->search<SampleData*>(query).onResult([this](QList<SampleData*> data){
			report(QtInfoMsg, "Searched data in store!");
			foreach (auto d, data)
				update(d);
		}, [this](QException &exception) {
			report(QtCriticalMsg, QString::fromUtf8(exception.what()));
		});
	}
}
