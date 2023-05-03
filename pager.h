#ifndef PAGER_H
#define PAGER_H

class Pager
{
public:
    Pager(std::vector<uint8_t> buffer, const std::vector<hsize_t>& dims, size_t data_size);

    size_t columnCount() const;
    size_t rowCount() const;

    size_t pageCount() const;
    size_t dataSize() const;
    std::vector<size_t> getHiDimByPage(size_t) const; // 得到指定页的高维度（低2维度当作表格）

    std::span<uint8_t> getPageData(size_t);
private:
    std::vector<uint8_t> _buffer;
    std::vector<size_t> _hi_dims;
    size_t _colCount{1};
    size_t _rowCount{1};
    size_t _data_size;
};

#endif