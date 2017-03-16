#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include "sampledata.h"

#include <QTreeWidget>
#include <QWidget>
#include <asyncdatastore.h>
#include <synccontroller.h>

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
	void dataChanged(int metaTypeId, const QString &key, bool wasDeleted);
	void dataResetted();

	void updateProgress(int count);
	void update(SampleData *data);
	void setup();

	void on_addButton_clicked();
	void on_deleteButton_clicked();
	void on_changeUserButton_clicked();
	void on_dataTreeWidget_itemSelectionChanged();
	void on_searchEdit_returnPressed();

private:
	Ui::MainWidget *ui;

	QtDataSync::AsyncDataStore *store;
	QtDataSync::SyncController *sync;
	QHash<int, QTreeWidgetItem*> items;
};

#endif // MAINWIDGET_H
