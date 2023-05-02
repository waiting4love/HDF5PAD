#include "prefix.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    initTree();
}

void MainWindow::initTree()
{
    ui->tree->setColumnCount(2);
    ui->tree->setHeaderLabels({tr("Name"),tr("Type")});
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionOpen_triggered()
{
    auto fileName = QFileDialog::getOpenFileName(this,
      tr("Open HDF5 Files"), "", tr("HDF5 Files (*.*)"));
    if(fileName.isNull() || fileName.isEmpty()) return;
    file_ptr = std::make_unique<HighFive::File>(fileName.toStdString());
    gotoPath("");
}

QString typeToStr(HighFive::ObjectType type)
{
    switch(type)
    {
    case HighFive::ObjectType::File:
        return QObject::tr("File");
    case HighFive::ObjectType::Group:
        return QObject::tr("Group");
    case HighFive::ObjectType::UserDataType:
        return QObject::tr("UserType");
    case HighFive::ObjectType::DataSpace:
        return QObject::tr("DataSpace");
    case HighFive::ObjectType::Dataset:
        return QObject::tr("Dataset");
    case HighFive::ObjectType::Attribute:
        return QObject::tr("Attribute");
    case HighFive::ObjectType::Other:
        return QObject::tr("Other");
    default:
        return QObject::tr("Unknown");
    }
}

QString datasetTypeStr(const HighFive::DataSet& ds)
{
    auto data_type = ds.getDataType();

    auto dims = ds.getDimensions();
    QStringList sl;
    std::transform(dims.begin(), dims.end(), std::back_inserter(sl), [](auto v){return QString::number(v);});
    return QString::fromStdString( HighFive::type_class_string(data_type.getClass()) ) + ": " + sl.join(L'×');
}

void appendGroupMember(QTreeWidgetItem* parent, const HighFive::Group& group)
{
    auto names = group.listObjectNames();
    for(const auto& name : names)
    {
        auto type = group.getObjectType(name);
        auto type_str = typeToStr(type);
        if(type == HighFive::ObjectType::Dataset)
        {
            type_str = type_str + "(" + datasetTypeStr(group.getDataSet(name)) + ")";
        }
        auto item = new QTreeWidgetItem(parent, QStringList{QString::fromStdString(name), type_str});
        if(type == HighFive::ObjectType::Group) // add sub items
        {
            appendGroupMember(item, group.getGroup(name));
        }
    }
}

template <typename Derivate>
void setTreeContent(QTreeWidget* treeWidget, const HighFive::NodeTraits<Derivate>& node)
{
    auto names = node.listObjectNames();
    QList<QTreeWidgetItem *> items;
    for(const auto& name : names)
    {
        auto type = node.getObjectType(name);
        auto type_str = typeToStr(type);
        if(type == HighFive::ObjectType::Dataset)
        {
            type_str = type_str + "(" + datasetTypeStr(node.getDataSet(name)) + ")";
        }

        auto item = new QTreeWidgetItem(static_cast<QTreeWidget *>(nullptr), QStringList{QString::fromStdString(name), type_str});

        if(type == HighFive::ObjectType::Group) // add sub items
        {
            appendGroupMember(item, node.getGroup(name));
        }

        items.append(item);
    }
    treeWidget->clear();
    treeWidget->insertTopLevelItems(0, items);
}

template <typename Derivate>
void showAttrib(QTableWidget* table, const HighFive::AnnotateTraits<Derivate>& obj)
{
    table->clear();
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels({QObject::tr("Name"), QObject::tr("Type"), QObject::tr("Value")});
    table->setRowCount((int)obj.getNumberAttributes());
    int row = 0;
    for(const auto &name : obj.listAttributeNames())
    {
        table->setItem(row, 0,  new QTableWidgetItem(QString::fromStdString(name)));

        auto attr = obj.getAttribute(name);
        auto data_type = attr.getDataType();
        table->setItem(row, 1,  new QTableWidgetItem(QString::fromStdString(data_type.string())));
        
        try{
            QString val;
            switch(data_type.getClass())
            {
            case HighFive::DataTypeClass::String:
                {
                    auto stsize = attr.getStorageSize();
                    std::string buff(stsize, 0);
                    attr.read(buff.data(), data_type);
                    val = QString::fromStdString(buff);
                }
                break;
            case HighFive::DataTypeClass::Integer:
                val = QString::number(attr.read<int>());
                break;
            case HighFive::DataTypeClass::Float:
                val = QString::number(attr.read<double>());
                break;
            case HighFive::DataTypeClass::Reference:
                val = QString::number(attr.read<size_t>(), 16).toUpper();
                break;
            case HighFive::DataTypeClass::VarLen:
                {
                    auto stsize = attr.getStorageSize();
                    QByteArray buff((int)stsize, 0);
                    attr.read(buff.data(), data_type);
                    QStringList sl;
                    std::transform(buff.begin(), buff.end(), std::back_inserter(sl), [](char c){ return QString::asprintf("%02X", (uint8_t)c); });
                    val = sl.join(' ');
                }
            default:
                break;
            }
            table->setItem(row, 2,  new QTableWidgetItem(val));
        }
        catch(...)
        {

        }

        row++;
    }
}

void MainWindow::gotoPath(const QString& path)
{
    ui->tree->clear();
    clearItemViewer();

    try{
        handlePath(path,
            [this](const HighFive::File& f){
                setTreeContent(ui->tree,f);
                ::showAttrib(ui->tableAttr, f);
            },
            [this](const HighFive::DataSet& d){
                ::showAttrib(ui->tableAttr, d);
                showData(d);
            },
            [this](const HighFive::Group& g){
                setTreeContent(ui->tree,g);
                ::showAttrib(ui->tableAttr, g);
            },
            [](){}
            );
        root_path = path;
        ui->edtPath->setText(path);
    }
    catch(const HighFive::Exception& ex) {
        QMessageBox::critical(this, tr("HDF5 PAD"),
                               ex.what(),
                               QMessageBox::Ok);
    }
}

void MainWindow::on_btnGo_clicked()
{
    auto path = ui->edtPath->text().trimmed();
    gotoPath(path);
}

QString getTreePath(QTreeWidgetItem *item)
{
    auto tokens = std::make_unique<QStringList>(); 
    while(item)
    {
        tokens->insert(0, item->text(0));
        item = item->parent();
    }
    return tokens->join('/');
}

void MainWindow::on_tree_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    if(!item) return;
    gotoPath(root_path + "/" + getTreePath(item));
}

void MainWindow::on_btnUp_clicked()
{
    if(root_path.isEmpty() || root_path == "/") return;
    int idx = root_path.lastIndexOf('/');
    auto path = root_path.left(idx);
    gotoPath(path);
}

void MainWindow::on_tree_itemSelectionChanged()
{
    auto items = ui->tree->selectedItems();
    if(items.empty())
    {
        clearItemViewer();
    }
    else
    {
        showItemViewer(root_path + "/" +getTreePath(items.first()));
    }
}

void MainWindow::clearItemViewer()
{
    ui->tableAttr->clearContents();
    ui->tableData->clear();
    ui->labelData->setText("");
}

void MainWindow::showStringData(const std::vector<uint8_t>& data, size_t valueSize, const std::vector<hsize_t>& dims)
{

}
void MainWindow::showDoubleData(const std::vector<uint8_t>& data, size_t valueSize, const std::vector<hsize_t>& dims)
{
    auto table = ui->tableData;
    if(dims.size() <= 2)
    {
        hsize_t row = 1;
        hsize_t col = 1;
        if(dims.size() > 0)
            row = dims[0];
        if(dims.size() > 1)
            col = dims[1];
        table->setColumnCount(col);
        table->setRowCount(row);

        size_t eleCount = std::min<size_t>(data.size()/valueSize, row*col);
        for(size_t i=0; i<eleCount; i++)
        {
            double val = 0;
            memcpy(&val, &data[i*valueSize], std::min(valueSize, sizeof(val)));
            auto r = i / col;
            auto c = i % col;
            table->setItem((int)r, (int)c,  new QTableWidgetItem(QString::number(val)));
        }
    }
}
void MainWindow::showIntData(const std::vector<uint8_t>& data, size_t valueSize, const std::vector<hsize_t>& dims)
{
    auto table = ui->tableData;
    if(dims.size() <= 2)
    {
        hsize_t row = 1;
        hsize_t col = 1;
        if(dims.size() > 0)
            row = dims[0];
        if(dims.size() > 1)
            col = dims[1];
        table->setColumnCount(col);
        table->setRowCount(row);

        size_t eleCount = std::min<size_t>(data.size()/valueSize, row*col);
        for(size_t i=0; i<eleCount; i++)
        {
            int val = 0;
            memcpy(&val, &data[i*valueSize], std::min(valueSize, sizeof(val)));
            auto r = i / col;
            auto c = i % col;
            table->setItem((int)r, (int)c,  new QTableWidgetItem(QString::number(val)));
        }
    }
}

QString MainWindow::getMatlabString(const HighFive::DataSet& dataset)
{
    auto data_type = dataset.getDataType();
    auto class_type = data_type.getClass();
    auto size = data_type.getSize();
    auto stsize = size * dataset.getElementCount();
    if(size == 0) return {};
    std::vector<uint8_t> buff(stsize);
    dataset.read(buff.data(), data_type);
    return getMatlabString(buff, size);
}

QString MainWindow::getMatlabString(const std::vector<uint8_t>& data, size_t valueSize)
{
    size_t eleCount = data.size()/valueSize;
    std::wstring str(eleCount+1, 0);
    for(size_t i=0; i<eleCount; i++)
    {
        memcpy(&str[i], &data[i*valueSize], std::min(valueSize, sizeof(str[i])));
    }
    return QString::fromStdWString(str);
}

bool isMatlabString(const HighFive::DataSet& dataset)
{
    std::string name = "MATLAB_class";
    if(dataset.hasAttribute(name))
    {
        auto attr = dataset.getAttribute(name);
        auto stsize = attr.getStorageSize();
        std::string buff(stsize, 0);
        attr.read(buff.data(), attr.getDataType());
        return buff == "char";
    }
    return false;
}

class MyRef : public HighFive::Reference
{
public:
    MyRef(const hobj_ref_t h5_ref)
    :HighFive::Reference(h5_ref){}
};

class MyTableRefItem : public QTableWidgetItem
{
public:
    using QTableWidgetItem::QTableWidgetItem;
    std::string path;
    HighFive::ObjectType type;
};

void MainWindow::showRefData(const std::vector<uint8_t>& data, size_t valueSize, const std::vector<hsize_t>& dims)
{
    auto table = ui->tableData;
    if(dims.size() <= 2)
    {
        hsize_t row = 1;
        hsize_t col = 1;
        if(dims.size() > 0)
            row = dims[0];
        if(dims.size() > 1)
            col = dims[1];
        table->setColumnCount(col);
        table->setRowCount(row);

        size_t eleCount = std::min<size_t>(data.size()/valueSize, row*col);
        for(size_t i=0; i<eleCount; i++)
        {
            hobj_ref_t val = 0;
            memcpy(&val, &data[i*valueSize], std::min(valueSize, sizeof(val)));

            auto item = new MyTableRefItem();

            MyRef ref(val);
            if(HighFive::ObjectType::Dataset == ref.getType(*file_ptr))
            {
                auto ds = ref.dereference<HighFive::DataSet>(*file_ptr);
                item->path  = ds.getPath();
                if(isMatlabString(ds))
                {
                    item->setText(getMatlabString(ds));
                }
                else
                {
                    item->setText(QString::fromStdString(item->path));
                }
            }
            else
            {
                auto grp = ref.dereference<HighFive::Group>(*file_ptr);
                item->path  = grp.getPath();
                item->setText(QString::fromStdString(item->path));
            }

            auto r = i / col;
            auto c = i % col;
            table->setItem((int)r, (int)c, item);
        }
    }
}

void MainWindow::showData(const HighFive::DataSet& dataset)
{
    auto table = ui->tableData;
    auto text = ui->labelData;
    table->clear();
    text->setText("");
    auto dims = dataset.getDimensions();
    auto data_type = dataset.getDataType();
    auto class_type = data_type.getClass();
    auto size = data_type.getSize();
    //auto stsize = dataset.getStorageSize();
    auto stsize = size * dataset.getElementCount();
    if(size == 0) return;
    std::vector<uint8_t> buff(stsize);
    dataset.read(buff.data(), data_type);

    switch(class_type)
    {
    case HighFive::DataTypeClass::String:
        showStringData(buff, size, dims);
        break;
    case HighFive::DataTypeClass::Float:
        showDoubleData(buff, size, dims);
        break;
    case HighFive::DataTypeClass::Integer:
        showIntData(buff, size, dims);
        if(isMatlabString(dataset)) {
            ui->labelData->setText(getMatlabString(buff, size));
        }
        break;
    case HighFive::DataTypeClass::Reference:
        showRefData(buff, size, dims);
        break;
    }
}

void MainWindow::showItemViewer(const QString& path)
{
    clearItemViewer();

    try {
        handlePath(path,
            [this](const HighFive::File& f){ ::showAttrib(ui->tableAttr, f);},
            [this](const HighFive::DataSet& d) {
                ::showAttrib(ui->tableAttr,d);
                showData(d);
            },
            [this](const HighFive::Group& g){ ::showAttrib(ui->tableAttr, g);},
            [](){}
        );
    }
    catch(const HighFive::Exception& ex) {
        QMessageBox::critical(this, tr("HDF5 PAD"),
                               ex.what(),
                               QMessageBox::Ok);
    }
}

void MainWindow::handlePath(const QString& path,
    std::function<void(const HighFive::File&)> hf,
    std::function<void(const HighFive::DataSet&)> hd,
    std::function<void(const HighFive::Group&)> hg,
    std::function<void()> hn
    )
{
    if(file_ptr) 
    {
        auto path_str = path.toStdString();
        if(path.startsWith('/'))
        {
            path_str.erase(0,1);
        }

        HighFive::ObjectType type;
        if(path_str.empty())
            type = HighFive::ObjectType::File;
        else
            type = file_ptr->getObjectType(path_str);
        switch(type)
        {
            case HighFive::ObjectType::File:
                hf(*file_ptr);
                return;
            case HighFive::ObjectType::Dataset:
                hd(file_ptr->getDataSet(path_str));
                return;
            case HighFive::ObjectType::Group:
                hg(file_ptr->getGroup(path_str));
                return;
            default:
                // unknown
                break;
        }
    }
    hn();
}

void MainWindow::on_tableData_cellDoubleClicked(int row, int column)
{
    auto item = ui->tableData->item(row, column);
    auto myitem = dynamic_cast<MyTableRefItem*>(item);
    if(myitem)
    {
        gotoPath(QString::fromStdString(myitem->path));
    }
}