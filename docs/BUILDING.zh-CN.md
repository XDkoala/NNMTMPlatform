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
