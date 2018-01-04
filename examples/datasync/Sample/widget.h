#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QtDataSync/syncmanager.h>
#include <QtDataSync/datastoremodel.h>

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
	Q_OBJECT

public:
	explicit Widget(const QString &setup, QWidget *parent = nullptr);
	~Widget();

private slots:
	void selectionChange(const QModelIndex &index);
	void updateState(QtDataSync::SyncManager::SyncState state);

	void on_addButton_clicked();
	void on_deleteButton_clicked();
	void on_clearButton_clicked();

	void on_accountButton_clicked();

private:
	const QString _setup;
	Ui::Widget *ui;
	QtDataSync::SyncManager *_manager;
	QtDataSync::DataStoreModel *_model;
};

#endif // WIDGET_H
