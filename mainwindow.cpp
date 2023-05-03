#include "prefix.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "helper.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    initTree();

    connect(ui->cbxDataPages, SIGNAL(currentIndexChanged(int)),  this, SLOT(showPage(int)));
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
    curr_dataset.reset();
    pagerPtr.reset();
    file_ptr = std::make_unique<HighFive::File>(fileName.toStdString());
    gotoPath("", GotoMode::Init);
}

void MainWindow::gotoPath(const QString& path, GotoMode mode)
{
    ui->tree->clear();
    clearItemViewer();
    if(!file_ptr) return;

    try{
        QString new_path = handlePath(
            *file_ptr,
            path,
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
        switch(mode)
        {
        case GotoMode::Init:
            back_paths.clear();
            forward_paths.clear();
            break;
        case GotoMode::Normal:
            if(!samePath(root_path, path)) {
                back_paths.push(root_path);
                forward_paths.clear();
            }
            break;
        case GotoMode::Back:
            if(!samePath(root_path, path)) {
                forward_paths.push(root_path);
            }
            break;
        case GotoMode::Forward:
            if(!samePath(root_path, path)) {
                back_paths.push(root_path);
            }
            break;
        }
        root_path = new_path;
        ui->edtPath->setText(new_path);
        updateUI();
    }
    catch(const HighFive::Exception& ex) {
        QMessageBox::critical(this, tr("HDF5 PAD"),
                               ex.what(),
                               QMessageBox::Ok);
    }
}

void MainWindow::on_actionBack_triggered()
{
    auto path = back_paths.pop();
    gotoPath(path, GotoMode::Back);
}

void MainWindow::on_actionForward_triggered()
{
    auto path = forward_paths.pop();
    gotoPath(path, GotoMode::Forward);
}

void MainWindow::on_actionCopy_triggered()
{
    auto tableData = dynamic_cast<QStandardItemModel*>(ui->tableView->model());
    if(!tableData) return;
    int row = tableData->rowCount();
    int col = tableData->columnCount();

    QStringList lines;
    for(int r=0; r<row; r++)
    {
        QStringList items;
        for(int c=0; c<col; c++)
        {
            items.append( tableData->item(r, c)->text() );
        }
        lines.append(items.join('\t'));
    }

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(lines.join('\n'));
}

void MainWindow::on_btnGo_clicked()
{
    auto path = ui->edtPath->text().trimmed();
    gotoPath(path, GotoMode::Normal);
}

void MainWindow::on_tree_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    if(!item) return;
    gotoPath(root_path + "/" + getTreePath(item), GotoMode::Normal);
}

void MainWindow::on_btnUp_clicked()
{
    if(root_path.isEmpty() || root_path == "/") return;
    int idx = root_path.lastIndexOf('/');
    auto path = root_path.left(idx);
    gotoPath(path, GotoMode::Normal);
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
    ui->tableView->setModel(nullptr);
    tableModel.reset();
    ui->labelData->setText("");
    ui->cbxDataPages->clear();
    curr_dataset.reset();
    pagerPtr.reset();
}

QStandardItem* MainWindow::createTableItem(const void* data, HighFive::DataTypeClass class_type, size_t size, HighFive::CompoundType* compType)
{   
    QString str;
    switch(class_type)
    {
    case HighFive::DataTypeClass::Reference:
        if(size == sizeof(hobj_ref_t)) {
            auto href = (hobj_ref_t*)data;
            if (hid_t res = H5Rdereference(file_ptr->getId(), H5P_DEFAULT, H5R_OBJECT, href); res >= 0) {
                MyTableRefItem *item = new MyTableRefItem();
                MyObj obj(res);
                auto obj_type = obj.getType();
                if(HighFive::ObjectType::Dataset == obj_type)
                {
                    auto& ds = reinterpret_cast<const HighFive::DataSet&>(obj);  // 不是的话强制转换 //ref.dereference<HighFive::DataSet>(*file_ptr);
                    item->path = ds.getPath();
                    str = getShortString(ds);
                }
                else
                {
                    auto& grp = reinterpret_cast<const HighFive::Group&>(obj);
                    item->path = grp.getPath();
                    str = QString::fromStdString(item->path);
                } 
                item->setText(str);
                item->type = obj_type;
                return item;
            }
        }
        break;
    case HighFive::DataTypeClass::Compound:
        if(compType!=nullptr) {
            QStringList sl;
            for(auto& m : compType->getMembers()) {
                auto sub_size = m.base_type.getSize();
                if(m.offset + sub_size <= size) {
                    sl.append(getDisplayString((const char*)data + m.offset, m.base_type.getClass(), m.base_type.getSize())); 
                } else {
                    sl.append("?");
                }
            }
            str = "{"+sl.join(',')+"}";
        }
        break;
    default:
        str = getDisplayString(data, class_type, size);
        break;
    }
    return new QStandardItem(str);
}

void MainWindow::updateUI()
{
    ui->actionBack->setEnabled(!back_paths.empty());
    ui->actionForward->setEnabled(!forward_paths.empty());

    auto tableData = dynamic_cast<QStandardItemModel*>(ui->tableView->model());
    ui->actionCopy->setEnabled( tableData && tableData->rowCount() * tableData->columnCount() > 0);
}

QString MainWindow::getShortString(const HighFive::DataSet& dataset)
{
    const static std::string name = "MATLAB_class";
    auto data_type = dataset.getDataType();
    auto class_type = data_type.getClass();
    auto size = data_type.getSize();
    auto eleCount = dataset.getElementCount();
    auto stsize = size * eleCount;
    std::string mat_class;
    if(dataset.hasAttribute(name))
    {
        auto attr = dataset.getAttribute(name);
        auto stsize = attr.getStorageSize();
        std::string buff(stsize, 0);
        attr.read(buff.data(), attr.getDataType());
        mat_class = buff;
    }

    std::unique_ptr<HighFive::CompoundType> compType;
    if(class_type == HighFive::DataTypeClass::Compound)
    {
        compType = std::make_unique<HighFive::CompoundType>(dataset.getDataType());
    }

    if(class_type == HighFive::DataTypeClass::Integer && mat_class=="char")
    {
        if(size == 0) return {};
        std::vector<uint8_t> buff(stsize);
        dataset.read(buff.data(), data_type);
        std::wstring str(eleCount+1, 0);
        for(size_t i=0; i<eleCount; i++)
        {
            memcpy(&str[i], &buff[i*size], std::min(size, sizeof(str[i])));
        }
        return QString::fromStdWString(str);
    }
    else if(mat_class == "cell" && eleCount < 3) // show little cell
    {
        if(size == 0) return {};
        std::vector<uint8_t> buff(stsize);
        dataset.read(buff.data(), data_type);
        QStringList sl;
        for (size_t i = 0; i < eleCount; i++)
        {
            std::unique_ptr<QStandardItem> s(createTableItem(&buff[i * size], class_type, size, compType.get()));
            sl.append(s->text());
        }
        return "["+sl.join(', ')+"]";
    }
    else if(eleCount == 1)
    {
        if(size == 0) return {};
        std::vector<uint8_t> buff(stsize);
        dataset.read(buff.data(), data_type);
        std::unique_ptr<QStandardItem> s(createTableItem(&buff[0], class_type, size, compType.get()));
        return s->text();
    }
    return QString::fromStdString(dataset.getPath());
}

void MainWindow::showData(const HighFive::DataSet& dataset)
{
    pagerPtr.reset();
    curr_dataset.reset();
    auto table = ui->tableView;
    auto text = ui->labelData;
    auto pages = ui->cbxDataPages;
    pages->clear();
    table->setModel(nullptr);
    text->setText("");
    auto dims = dataset.getDimensions();
    auto data_type = dataset.getDataType();
    auto class_type = data_type.getClass();
    auto size = data_type.getSize();
    auto eleCount = dataset.getElementCount();
    auto stsize = size * eleCount;
    if(size == 0) return;

    std::vector<uint8_t> buff(stsize);
    dataset.read(buff.data(), data_type);

    curr_dataset = std::make_unique<HighFive::DataSet>(dataset);
    pagerPtr = std::make_unique<Pager>(std::move(buff), dims, size);
    for (size_t i = 0, c = std::min<>(pagerPtr->pageCount(), 100ull); i < c; i++)
    {
        auto hidim = pagerPtr->getHiDimByPage(i);
        QStringList sl;
        std::transform(
            hidim.rbegin(), hidim.rend(), std::back_inserter(sl),
            [](auto d){ return QString::number(d + 1); });
        QString pageName = "[:,:,"+sl.join(',')+"]";
        ui->cbxDataPages->addItem(pageName, i);
    }

    if(class_type == HighFive::DataTypeClass::Compound)
    {
        std::unique_ptr<HighFive::CompoundType> compType;
        compType = std::make_unique<HighFive::CompoundType>(std::move(data_type));
                QStringList sl;
        for(auto& m : compType->getMembers())
        {
            sl.append(QString::fromStdString(m.name));
        }
        text->setText( "Compound: " + sl.join(';') );
    }
    else
    {
        text->setText(getShortString(*curr_dataset));
    }

    showPage(0);
}

void MainWindow::showPage(int idx)
{
    if(idx < 0) return;
    auto table = ui->tableView;
    table->setModel(nullptr);
    if(!pagerPtr || !curr_dataset) return;
    auto buff = pagerPtr->getPageData(idx);
    auto row = pagerPtr->rowCount();
    auto col = pagerPtr->columnCount();
    auto size = pagerPtr->dataSize();
    auto eleCount = buff.size() / size;

    tableModel = std::make_unique<QStandardItemModel>();

    auto data_type = curr_dataset->getDataType();
    auto class_type = data_type.getClass();
    std::unique_ptr<HighFive::CompoundType> compType;
    if(class_type == HighFive::DataTypeClass::Compound)
    {
        compType = std::make_unique<HighFive::CompoundType>(std::move(data_type));
    }

    for(size_t r=0; r<row; r++)
    {
        QList<QStandardItem*> rowItems;
        for(size_t c=0; c<col; c++)
        {
            size_t idx = r*col+c;
            auto s = createTableItem(&buff[idx * size], class_type, size, compType.get());
            rowItems.append(s);
        }
        tableModel->appendRow(rowItems);
    }

    table->setModel(tableModel.get());
    updateUI();
}

void MainWindow::showItemViewer(const QString& path)
{
    clearItemViewer();
    if(!file_ptr) return;
    try {
        handlePath(
            *file_ptr,
            path,
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

void MainWindow::on_tableView_doubleClicked(const QModelIndex &index)
{
    auto tableData = dynamic_cast<QStandardItemModel*>(ui->tableView->model());
    if(!tableData) return;

    auto item = tableData->item(index.row(), index.column());
    auto myitem = dynamic_cast<MyTableRefItem*>(item);
    if(myitem)
    {
        gotoPath(QString::fromStdString(myitem->path), GotoMode::Normal);
    }
}