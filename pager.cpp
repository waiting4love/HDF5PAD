#include "prefix.h"
#include "pager.h"

Pager::Pager(std::vector<uint8_t> buffer, const std::vector<hsize_t>& dims, size_t data_size)
:_buffer(std::move(buffer)), _data_size(data_size)
{
    // std::vector<hsize_t> _hi_dims;
    // size_t _colCount;
    // size_t _rowCount;

    if (dims.size() == 1)
    {
        _colCount = dims.back();
    }
    else if (dims.size() > 1)
    {
        _colCount = dims.back();
        _rowCount = dims[dims.size()-2];
    }

    if(dims.size() > 2)
    {
        _hi_dims.insert(_hi_dims.end(), dims.begin(), dims.end() - 2);
    }
}

size_t Pager::columnCount() const
{
    return _colCount;
}

size_t Pager::rowCount() const
{
    return _rowCount;
}

size_t Pager::pageCount() const
{
    auto pageCountByDim = std::accumulate(_hi_dims.begin(), _hi_dims.end(), size_t{1u}, std::multiplies<size_t>());
    auto bytePerPage = _data_size*_colCount*_rowCount;
    auto pageCountByBuff =  (_buffer.size() + bytePerPage - 1)/bytePerPage;
    return std::min<>(pageCountByDim, pageCountByBuff);
}

size_t Pager::dataSize() const
{
    return _data_size;
}

std::vector<size_t> Pager::getHiDimByPage(size_t pageIdx) const // 得到指定页的高维度（低2维度当作表格）
{
    std::vector<size_t> res;
    for(auto itr = _hi_dims.rbegin(); itr != _hi_dims.rend(); ++itr)
    {
        auto d = *itr;
        res.push_back(pageIdx % d);
        pageIdx /= d;
    }
    std::reverse(res.begin(), res.end());
    return res;
}

std::span<uint8_t> Pager::getPageData(size_t pageIdx)
{
    auto size = _buffer.size();
    auto bytePerPage = _data_size*_colCount*_rowCount;
    auto begin = std::min(size, bytePerPage * pageIdx);
    auto end = std::min(size, bytePerPage * pageIdx + bytePerPage);
    return std::span<uint8_t>(_buffer.begin()+begin, _buffer.begin()+end);
}