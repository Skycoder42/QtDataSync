#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QtDataSync/datastoremodel.h>

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
	Q_OBJECT

public:
	explicit Widget(QWidget *parent = nullptr);
	~Widget();

	void report(QtMsgType type, const QString &message);

private slots:
	void selectionChange(const QModelIndex &index);

	void on_addButton_clicked();
	void on_deleteButton_clicked();
	void on_clearButton_clicked();

private:
	Ui::Widget *ui;
	QtDataSync::DataStoreModel *_model;
};

#endif // WIDGET_H
