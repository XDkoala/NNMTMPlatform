# Windows 构建指南

## 需要的工具

- Visual Studio 2022 Build Tools，安装“使用 C++ 的桌面开发”工作负载。
- CMake 3.24 或更高版本。
- Ninja。
- 与 MSVC x64 兼容的 Qt 6：Core、Gui、Widgets。图表由项目使用 QPainter 绘制，不依赖 Qt Charts。
- LibTorch。CPU 版可直接使用 CPU；CUDA 版需要兼容的 NVIDIA 驱动和 CUDA Toolkit。
- OpenCV：core、imgproc、imgcodecs。项目默认通过 vcpkg 安装路径查找。

只需要 MSVC 编译工具，不需要用 Visual Studio IDE 打开 `.sln`。

## 本机预设

复制：

```powershell
Copy-Item CMakeUserPresets.example.json CMakeUserPresets.json
```

编辑 `CMakeUserPresets.json`，设置：

- `CMAKE_MAKE_PROGRAM`：`ninja.exe` 的完整路径。
- `CMAKE_PREFIX_PATH`：以分号分隔 Qt、LibTorch 和 vcpkg `installed/x64-windows` 路径。
- `TORCH_CUDA_ARCH_LIST`：可选，只在 CUDA 架构无法自动识别时设置。

`CMakeUserPresets.json` 包含本机路径，已被 Git 忽略。

## 配置、构建与测试

在“x64 Native Tools Command Prompt for VS 2022”中运行：

```powershell
cmake --preset local-windows-release
cmake --build --preset local-windows-release
ctest --test-dir build/local-windows-release --output-on-failure
```

调试版将 `release` 替换为 `debug`。

## 常见问题

### `stdlib.h` 或其他系统头文件找不到

当前终端没有初始化 MSVC 环境。改用 x64 Native Tools Command Prompt，或先运行 Visual Studio 的 `Launch-VsDevShell.ps1`。

### Qt、Torch 或 OpenCV 找不到

检查 `CMAKE_PREFIX_PATH`中的每个路径是否指向库的安装根目录，并确保所有库都是 x64 MSVC 版。修改预设后重新执行 CMake 配置。

### CUDA 可用但程序回退到 CPU

1. 确认使用的是 CUDA 版 LibTorch。
2. 运行 `ctest` 查看 `cuda_runtime` 的具体错误。
3. 确认 NVIDIA 驱动支持 LibTorch 所需的 CUDA 运行时。
4. 新架构显卡可在本机预设中设置 `TORCH_CUDA_ARCH_LIST`。

CUDA Toolkit 的小版本与 LibTorch 包名不完全一致不一定代表失败，应以 `cuda_runtime` 的真实运算结果为准。

### 程序启动时缺少 DLL

请从 CMake 生成的目录启动程序。正常构建会自动复制 Qt、OpenCV 和 LibTorch 运行库；如果手动移动 `.exe`，需要同时携带这些依赖。

## Windows 便携包维护

`scripts/Package-Windows.ps1` 默认打包 CPU；指定 `-Backend cuda` 打包
`windows-x64-cuda128`。两个后端必须使用独立的 Release 构建目录，
分别链接 LibTorch 2.9.0 CPU/CUDA 12.8，不能混用 DLL。
GPU 的普通 ZIP 超过 GitHub 单附件限制，发布包使用 `-ArchiveFormat 7z`
和 `-SevenZipPath <7zr.exe或7z.exe路径>`；验证脚本也需提供 `-SevenZipPath`。
7-Zip 仅作为构建工具使用，不会捆绑到程序包中。
首次 GPU 打包构建建议使用 `cmake --build <构建目录> --parallel 1`，
避免多个目标同时部署到同一目录时读取未复制完的大型 CUDA DLL。

脚本要求提供 Qt 安装和许可证、vcpkg 安装、MSVC 可再分发 CRT、dumpbin、
对应 Qt 6.10.1 源码，以及匹配后端的官方 PyTorch wheel 许可证。
CUDA 版额外通过 `-NvidiaLicenseDirectory` 提供官方 CUDA 12.8 EULA、
cuDNN 9.10.2 EULA（含补充条款）、CUPTI 12.8 许可文档。
具体参数和必需文件名见脚本开头。

`scripts/Fetch-TorchLicenseWheel.py` 可仅下载 CUDA 官方 wheel 内的
LICENSE/NOTICE，输出小型 ZIP 给 `-TorchLicenseWheel` 使用，避免下载整个 wheel。
它不是可安装的 Python 包。

输出目录必须是项目内的新目录，脚本不覆盖已有包。`-SourceRef` 默认 HEAD；
给已有 Release 添加不同后端时可指定其 tag，应用源码必须与该 tag 一致。
清单分别记录应用源码提交和打包脚本提交。发布时保留同一 Release 的 Qt 源码附件。

打包后用 `scripts/Validate-WindowsPackage.ps1` 提供 `-ArchivePath`、
`-BuildDirectory` 和新的 `-OutputDirectory` 验证。GPU 包指定 `-Device cuda`，
将在真实 NVIDIA GPU 上检查 CUDA、强制 CPU 和隐藏 GPU 后自动回退。
验证进程隔离开发工具 PATH；隐藏 GPU 不等同于没有驱动的干净系统测试。
