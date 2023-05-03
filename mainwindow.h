#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel> 
#include "pager.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    // ui中使用了QMetaObject::connectSlotsByName(MainWindow)，直接按规则命令就会自动连接
public slots:
    void on_actionOpen_triggered();
    void on_actionBack_triggered();
    void on_actionForward_triggered();
    void on_actionCopy_triggered();
    void on_btnGo_clicked();
    void on_btnUp_clicked();
    void on_tree_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_tree_itemSelectionChanged();
    void on_tableView_doubleClicked(const QModelIndex &index);
    void showPage(int idx);

private:
    Ui::MainWindow *ui;
    std::unique_ptr<HighFive::File> file_ptr;
    QString root_path;
    QStack<QString> back_paths;
    QStack<QString> forward_paths;

    std::unique_ptr<HighFive::DataSet> curr_dataset;
    std::unique_ptr<Pager> pagerPtr;
    std::unique_ptr<QStandardItemModel> tableModel;

private:
    // back: root_path入forward_paths, back_paths出栈, 更新按钮状态
    // go: root_path入back_paths，forward_path清空, 更新按钮状态
    // forward: root_path入back_paths, forward_path出栈, 更新按钮状态
    enum class GotoMode { Init, Normal, Back, Forward };
    void gotoPath(const QString& path, GotoMode mode);
    void initTree();
    void clearItemViewer();
    void showItemViewer(const QString& path);
    void showData( const HighFive::DataSet& dataset);
    QString getShortString(const HighFive::DataSet& dataset);
    QStandardItem* createTableItem(const void* data, HighFive::DataTypeClass class_type, size_t size, HighFive::CompoundType* compType=nullptr);
    void updateUI();
};

#endif // MAINWINDOW_H
