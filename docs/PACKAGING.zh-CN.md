# Windows 便携包构建

普通用户直接下载 Release 的 Windows ZIP 即可。以下步骤供维护者重复制作包。

## CPU 构建

使用独立构建目录和官方 Release 版 CPU LibTorch 2.9.0，避免混入旧 CUDA DLL。
在已经初始化 MSVC 的终端中执行（替换本机路径）：

```powershell
cmake -S . -B build/windows-portable-cpu -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  '-DCMAKE_PREFIX_PATH=C:/Qt/6.10.1/msvc2022_64;C:/libtorch-cpu;C:/vcpkg/installed/x64-windows'
cmake --build build/windows-portable-cpu
ctest --test-dir build/windows-portable-cpu --output-on-failure
```

Qt Charts 已移除；图表由 `PlotWidget.h` 使用 QPainter 绘制。

## 打包输入

- 已部署运行库的 CPU 构建目录。
- Qt 6.10.1 安装目录和 Qt 安装器的 Licenses 目录。
- vcpkg `installed/x64-windows` 目录。
- MSVC x64 Release CRT 目录，如 `VC/Redist/MSVC/<version>/x64/Microsoft.VC143.CRT`。
- 官方 `torch-2.9.0+cpu-cp311-cp311-win_amd64.whl`：仅用于提取完整 LICENSE/NOTICE，无需安装 Python。
- Qt 官方 `qtbase-everywhere-src-6.10.1.tar.xz` 和 `qtsvg-everywhere-src-6.10.1.tar.xz`，置于单独目录。

LibTorch/torch 下载源为 `https://download.pytorch.org/`；Qt 源码下载源为
`https://download.qt.io/archive/qt/6.10/6.10.1/submodules/`。
脚本内校验 Qt 源码的官方 SHA256，并随包保留许可证和构建配置。

```powershell
./scripts/Package-Windows.ps1 `
  -BuildDirectory build/windows-portable-cpu `
  -QtDirectory C:/Qt/6.10.1/msvc2022_64 `
  -QtLicenseDirectory C:/Qt/Licenses `
  -VcpkgDirectory C:/vcpkg/installed/x64-windows `
  -CrtDirectory C:/path/to/Microsoft.VC143.CRT `
  -TorchLicenseWheel C:/path/to/torch-cpu.whl `
  -QtSourceDirectory C:/path/to/qt-sources `
  -DumpbinPath C:/path/to/MSVC/bin/Hostx64/x64/dumpbin.exe `
  -OutputDirectory dist/release
```

脚本不覆盖已有包。重新打包请使用新的输出目录。升级依赖版本时，需要一并更新脚本中的源码哈希和第三方说明。

## 发布内容与验证

将 Windows ZIP、Qt 源码 ZIP 和各自的 `.sha256` 上传到同一个 Release。
Windows ZIP 不包含编译缓存、测试程序或个人模型数据。
脚本遍历 EXE/DLL 的导入表，自动补齐 OpenCV 图像编解码库等间接依赖。

脚本会在仅含 Windows 系统目录的 PATH 下启动真实程序，并从不同的工作目录
验证 `portable.ini` 能让数据目录生成在 EXE 旁边。检查成功后才压缩。
`manifest.json` 记录源提交、文件大小和 SHA256；ZIP 外的 `.sha256` 用于校验下载。

发布前还应把 ZIP 解压到新目录，重复启动和训练/推理检查。
可使用 `scripts/Validate-WindowsPackage.ps1 -ArchivePath <zip> -BuildDirectory <build>`：
脚本核对每个文件的 SHA256，并在仅使用包内 DLL 与 Windows 系统库的环境中运行真实 GUI 和 ANN/CNN 完整流程测试。
这些本机隔离检查不能替代另一台干净 Windows 机器上的验证。
