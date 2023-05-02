# HDF5 PAD
MATLAB的MAT(v7)文件使用HDF5来存放，所以写了一个工具研究研究MAT按什么结构存放到HDF5中的。顺便学习一下HDF5的用法，感谢[HighFive](https://github.com/BlueBrain/HighFive)项目，让我少走了很多弯路。

## 编译：

准备这些东西

1. CMake
2. QT
3. [HDF5](https://github.com/HDFGroup/hdf5)
4. clone本项目后执行`git submodule update --init`会自动下载HighFive

编译HDF5，得到DLL和LIB，把这些文件放到`3rdparty/hdf5/bin`中，并把HDF5的所有头文件放到`3rdparty/hdf5/include`中（注意别忘了`H5pubconf.h`这个文件）

注：用vcpkg来管理第三方库应该是个更好的主意，有时间简化一下

可以用CMAKE编译了。