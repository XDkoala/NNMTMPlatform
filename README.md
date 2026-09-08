# NNMTMPlatform

NNMTMPlatform 是一个使用 C++、Qt Widgets、LibTorch 和 OpenCV 编写的本地神经网络模型管理与训练工具。项目最初作为大一课程设计使用 Visual Studio 工程构建，目前正在迁移为编辑器无关的 CMake 工程。

## 当前功能

- 数值回归、数值二分类和数值多分类
- 图像二分类和图像多分类
- ANN/CNN 模型训练、测试与推理
- 模型元数据、权重和标签映射的本地保存
- 训练损失曲线、回归分析和分类分析界面
- 模型搜索、筛选、草稿箱和回收站

## 构建依赖

- 支持 C++17 的 MSVC 工具链
- CMake 3.24 或更高版本
- Ninja
- Qt 6：Core、Gui、Widgets、Charts
- LibTorch
- OpenCV：core、imgproc、imgcodecs
- 与 LibTorch 构建版本兼容的 CUDA Toolkit（CUDA 版 LibTorch 才需要）

不需要安装或打开 Visual Studio IDE，但 Windows 版依赖仍建议使用 MSVC Build Tools 编译。

## 配置本机路径

复制 `CMakeUserPresets.example.json` 为 `CMakeUserPresets.json`，然后将示例路径替换为本机的 Qt、LibTorch、OpenCV 和 Ninja 安装位置。`CMakeUserPresets.json` 包含本机路径，不会提交到 Git。

请先进入已经初始化 MSVC 环境的终端，例如“x64 Native Tools Command Prompt”，然后执行：

```powershell
cmake --preset local-windows-release
cmake --build --preset local-windows-release
```

生成的程序位于：

```text
build/local-windows-release/NNMTMPlatform.exe
```

CMake 会在构建后复制所需的 Qt、OpenCV 和 LibTorch 运行库。

也可以使用 Qt Creator 直接打开根目录的 `CMakeLists.txt`，选择 MSVC Qt Kit 后构建。

## 目录结构

```text
src/app/                 程序入口
src/ui/                  Qt 界面
src/core/                模型元数据、持久化与后台任务
src/models/              神经网络、ANN 和 CNN 实现
resources/images/        项目图片资源
docs/                    原课程需求和设计资料
legacy/visual-studio/    仅供参考的旧 Visual Studio 工程
examples/                后续加入的小型示例数据
tests/                   后续加入的自动化测试
```

## 本地数据

完整训练集、模型权重、模型元数据以及构建产物均被 `.gitignore` 排除。它们仍保留在原本的本地目录，不会因为 Git 或 CMake 迁移而被删除。

## 重构状态

当前已完成仓库清理、CMake/Ninja 构建迁移和第一轮源码目录整理。训练正确性、线程生命周期、图片流式加载、测试与完整文档将在后续阶段继续处理。

## License

许可证尚未确定。在添加明确许可证之前，本项目不授予复制、修改或再分发许可。
