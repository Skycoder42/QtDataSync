#include "mainwidget.h"
#include "sampledata.h"
#include "setupdialog.h"
#include "ui_mainwidget.h"
#include <QInputDialog>
#include <QMessageBox>
#include <cachingdatastore.h>
#include <setup.h>
#include <wsauthenticator.h>

static QtMessageHandler prevHandler;
static void filterLogger(QtMsgType type, const QMessageLogContext &context, const QString &msg);

MainWidget::MainWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::MainWidget),
	store(nullptr),
	sync(nullptr),
	model(nullptr)
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

void MainWidget::selectionChange(const QModelIndex &index)
{
	SampleData* data = model->object<SampleData*>(index);
	model->loadObject<SampleData*>(index).onResult(this, [](SampleData* d){
		qDebug() << "loaded successfully" << d;
		d->deleteLater();
	}, [this](const QException &e) {
		report(QtCriticalMsg, QString::fromUtf8(e.what()));
	});

	if(data) {
		ui->idSpinBox->setValue(data->id);
		ui->titleLineEdit->setText(data->title);
		ui->detailsLineEdit->setText(data->description);
	} else {
		ui->idSpinBox->clear();
		ui->titleLineEdit->clear();
		ui->detailsLineEdit->clear();
	}
}

void MainWidget::setup()
{
	if(SetupDialog::setup(this)) {
		store = new QtDataSync::AsyncDataStore(this);

		sync = new QtDataSync::SyncController(this);
		connect(sync, &QtDataSync::SyncController::syncStateChanged,
				this, [](QtDataSync::SyncController::SyncState state) {
			qDebug() << state;
		});
		connect(sync, &QtDataSync::SyncController::syncOperationsChanged,
				this, &MainWidget::updateProgress);
		connect(sync, &QtDataSync::SyncController::authenticationErrorChanged,
				this, [this](QString error) {
			if(error.isEmpty())
				qDebug() << "auth error cleared";
			else
				QMessageBox::critical(this, tr("Authentication error"), error);
		});
		qDebug() << sync->syncState();
		if(!sync->authenticationError().isEmpty())
			qDebug() << "auth error:" << sync->authenticationError();
		connect(ui->reloadButton, &QPushButton::clicked,
				sync, &QtDataSync::SyncController::triggerSync);
		connect(ui->resyncButton, &QPushButton::clicked,
				sync, &QtDataSync::SyncController::triggerResync);

		model = new QtDataSync::DataStoreModel(store, this);//TODO here
		model->setTypeId<SampleData*>();
		ui->dataTreeView->setModel(model);
		connect(ui->dataTreeView->selectionModel(), &QItemSelectionModel::currentChanged,
				this, &MainWidget::selectionChange);

		//caching test
		try {
			QtDataSync::CachingDataStore<SampleData*, int> cacheStore(nullptr, true);
			qDebug() << "caching store test:" << cacheStore.keys();
		} catch(QException &exception) {
			report(QtCriticalMsg, QString::fromUtf8(exception.what()));
		}
	} else
		qApp->quit();
}

void MainWidget::on_addButton_clicked()
{
	auto data = new SampleData(this);
	data->id = ui->idSpinBox->value();
	data->title = ui->titleLineEdit->text();
	data->description = ui->detailsLineEdit->text();

	store->save<SampleData*>(data).onResult(this, [this, data](){
		report(QtInfoMsg, QStringLiteral("Data with id %1 saved!").arg(data->id));
		data->deleteLater();
	}, [this, data](const QException &exception) {
		report(QtCriticalMsg, QString::fromUtf8(exception.what()));
		data->deleteLater();
	});
}

void MainWidget::on_deleteButton_clicked()
{
	auto index = ui->dataTreeView->currentIndex();
	auto id = model->key(index);
	if(!id.isNull()) {
		store->remove<SampleData*>(id).onResult(this, [this, id](bool success){
			if(success)
				report(QtInfoMsg, QStringLiteral("Data with id %1 removed!").arg(id));
			else
				report(QtInfoMsg, QStringLiteral("No entry with id %1 found!").arg(id));
		}, [this](const QException &exception) {
			report(QtCriticalMsg, QString::fromUtf8(exception.what()));
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
		task.onResult(this, [auth](){
			qDebug() << "Changed userId to" << auth->userIdentity();
			auth->deleteLater();
		});
	}
}

static void filterLogger(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
	if(QByteArray(context.category).startsWith("qtdatasync.")) {
		auto window = qApp->property("__mw").value<MainWidget*>();
		if(window) {
			auto rMsg = qFormatLogMessage(type, context, msg);
			window->report(type, rMsg);
			return;
		}
	}

	prevHandler(type, context, msg);
}
