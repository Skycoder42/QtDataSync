#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include "sampledata.h"

#include <QTreeWidget>
#include <QWidget>
#include <asyncdatastore.h>

namespace Ui {
class MainWidget;
}

class MainWidget : public QWidget
{
	Q_OBJECT

public:
	explicit MainWidget(QWidget *parent = nullptr);
	~MainWidget();

protected:
	void showEvent(QShowEvent *event) override;

private slots:
	void on_addButton_clicked();
	void on_deleteButton_clicked();

	void report(bool success, const QString &message);
	void update(SampleData *data);
	void reload();

	void setup();

private:
	Ui::MainWidget *ui;

	QtDataSync::AsyncDataStore *store;
	QHash<int, QTreeWidgetItem*> items;
};

#endif // MAINWIDGET_H
