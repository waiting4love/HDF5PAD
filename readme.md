# HDF5 PAD
MATLAB的MAT(v7.3)文件使用HDF5来存放，所以写了一个工具研究研究MAT按什么结构存放到HDF5中的。顺便学习一下HDF5的用法，感谢[HighFive](https://github.com/BlueBrain/HighFive)项目，让我少走了很多弯路。

## 编译：

准备这些东西

1. CMake
2. QT5.6 (我现在编译的版本，其它版本别太离谱的话问题应该不大)
3. VCPKG

用VCPKG安装HDF5和HighFive，下面的命令行是安装x64版本的:
`vcpkg install hdf5:x64-windows`
`vcpkg install highfive:x64-windows`

CMAKE编译时加入参数 `"-DCMAKE_TOOLCHAIN_FILE=D:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake"`，请修改你的路径。如果用VSCODE的话，在`.vscode/settings.json`里可以配置。