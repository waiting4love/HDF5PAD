#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

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
    void on_btnGo_clicked();
    void on_btnUp_clicked();
    void on_tree_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_tree_itemSelectionChanged();
    void on_tableData_cellDoubleClicked(int row, int column);
private:
    Ui::MainWindow *ui;
    std::unique_ptr<HighFive::File> file_ptr;
    QString root_path;

private:
    void gotoPath(const QString& path);
    void initTree();
    void clearItemViewer();
    void showItemViewer(const QString& path);
    void handlePath(const QString& path,
        std::function<void(const HighFive::File&)> hf,
        std::function<void(const HighFive::DataSet&)> hd,
        std::function<void(const HighFive::Group&)> hg,
        std::function<void()> hn
        );
    void showData( const HighFive::DataSet& dataset);
    void showStringData(const std::vector<uint8_t>& data, size_t valueSize, const std::vector<hsize_t>& dims);
    void showDoubleData(const std::vector<uint8_t>& data, size_t valueSize, const std::vector<hsize_t>& dims);
    void showIntData(const std::vector<uint8_t>& data, size_t valueSize, const std::vector<hsize_t>& dims);
    void showRefData(const std::vector<uint8_t>& data, size_t valueSize, const std::vector<hsize_t>& dims);
    QString getMatlabString(const std::vector<uint8_t>& data, size_t valueSize);
    QString getMatlabString(const HighFive::DataSet& dataset);
};

#endif // MAINWINDOW_H
