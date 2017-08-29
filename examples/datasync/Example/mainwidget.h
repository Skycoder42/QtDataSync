#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include "sampledata.h"

#include <QTreeWidget>
#include <QWidget>
#include <asyncdatastore.h>
#include <synccontroller.h>
#include <datastoremodel.h>

namespace Ui {
class MainWidget;
}

class MainWidget : public QWidget
{
	Q_OBJECT

public:
	explicit MainWidget(QWidget *parent = nullptr);
	~MainWidget();

public slots:
	void report(QtMsgType type, const QString &message);

private slots:
	void updateProgress(int count);
	void selectionChange(const QModelIndex &index);
	void setup();

	void on_addButton_clicked();
	void on_deleteButton_clicked();
	void on_changeUserButton_clicked();

private:
	Ui::MainWidget *ui;

	QtDataSync::AsyncDataStore *store;
	QtDataSync::SyncController *sync;
	QtDataSync::DataStoreModel *model;
};

#endif // MAINWIDGET_H
