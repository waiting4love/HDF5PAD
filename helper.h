#ifndef HELPER_H
#define HELPER_H

class MyObj: public HighFive::Object
{
public:
    explicit MyObj(hid_t id)
    :HighFive::Object(id){}
};

class MyTableRefItem : public QTableWidgetItem
{
public:
    using QTableWidgetItem::QTableWidgetItem;
    std::string path;
    HighFive::ObjectType type;
};

QString getDisplayString(const void* data, HighFive::DataTypeClass class_type, size_t size)
{
    QString str;
    switch(class_type)
    {
    case HighFive::DataTypeClass::String:
        str = QString::fromLocal8Bit((const char*)data, (int)size);
        break;
    case HighFive::DataTypeClass::Float:
        if(size == sizeof(double)){
            str = QString::number(*(double*)data);
        } else if(size == sizeof(float)) {
            str = QString::number(*(float*)data);
        }
        break;
    case HighFive::DataTypeClass::Integer:
        if(size == sizeof(int64_t)) {
            str = QString::number(*(int64_t*)data);
        } else if(size == sizeof(int32_t)) {
            str = QString::number(*(int32_t*)data);
        } else if(size == sizeof(int16_t)) {
            str = QString::number(*(int16_t*)data);
        } else if(size == sizeof(int8_t)) {
            str = QString::number(*(int8_t*)data);
        }
        break;
    default:
        str = QString::fromStdString(HighFive::type_class_string(class_type));
        break;
    }
    return str;
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
        auto class_type= data_type.getClass();
        auto size = data_type.getSize();
        auto eleCount = attr.getMemSpace().getElementCount();

        table->setItem(row, 1,  new QTableWidgetItem(QString::fromStdString(HighFive::type_class_string(class_type))));
        
        try{
            std::vector<uint8_t> buff(size * std::max<size_t>(1,eleCount), 0);
            attr.read(buff.data(), data_type);

            QString val;            
            if(class_type == HighFive::DataTypeClass::VarLen)
            {
                QStringList sl;
                std::transform(buff.begin(), buff.end(), std::back_inserter(sl), [](char c){ return QString::asprintf("%02X", (uint8_t)c); });
                val = sl.join(' ');
            }
            else
            {
                val = getDisplayString(&buff[0], class_type, size);
            }
            table->setItem(row, 2,  new QTableWidgetItem(val));
        }
        catch(...)
        {

        }

        row++;
    }
}

bool samePath(const QString& p1, const QString& p2)
{
    auto path_str1 = p1.toStdString();
    if(p1.startsWith('/'))
    {
        path_str1.erase(0,1);
    }

    auto path_str2 = p2.toStdString();
    if(p2.startsWith('/'))
    {
        path_str2.erase(0,1);
    }

    return path_str1 == path_str2;
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


QString handlePath(const HighFive::File& file, const QString& path,
    std::function<void(const HighFive::File&)> hf,
    std::function<void(const HighFive::DataSet&)> hd,
    std::function<void(const HighFive::Group&)> hg,
    std::function<void()> hn
    )
{
    QString res = path;
    auto path_str = path.toStdString();
    if(path.startsWith('/'))
    {
        path_str.erase(0,1);
    }

    HighFive::ObjectType type;
    if(path_str.empty())
        type = HighFive::ObjectType::File;
    else
        type = file.getObjectType(path_str);
    switch(type)
    {
        case HighFive::ObjectType::File:
            hf(file);
            res = "";
            return res;
        case HighFive::ObjectType::Dataset:
        {
            auto ds = file.getDataSet(path_str);
            hd(ds);
            res = QString::fromStdString(ds.getPath());
            return res;
        }
        case HighFive::ObjectType::Group:
        {
            auto gp = file.getGroup(path_str);
            hg(gp);
            res = QString::fromStdString(gp.getPath());
            return res;
        }
        default:
            // unknown
            break;
    }
    hn();
    return res;
}

#endif