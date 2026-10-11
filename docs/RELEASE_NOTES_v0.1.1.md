# NNMTMPlatform v0.1.1

新增 Windows 可运行便携包，下载后无需编译。

## Windows 使用

1. 下载 `NNMTMPlatform-v0.1.1-windows-x64-cpu.zip`。
2. 完整解压到可写目录，双击 `Start.cmd` 或 `NNMTMPlatform.exe`。
3. 用 `examples/numeric/` 中的示例开始训练和推理。

运行库已包含，无需安装 Visual Studio、Qt、Python、LibTorch 或 CUDA。
系统要求：Windows 10/11 x64，支持 AVX2 的处理器。建议至少 8 GB 内存。
此包使用 CPU。GPU 版见下文，两个下载项互不替换。

模型数据保存在程序旁的 `data/`，更新或移动程序前请备份。

## Windows CUDA GPU 版

下载 `NNMTMPlatform-v0.1.1-windows-x64-cuda128.7z`，用 7-Zip 或支持 7z
的工具完整解压后双击
`Start.cmd` 或 `NNMTMPlatform.exe`。解压需要约 5 GB 空间。
GPU 运行库的普通 ZIP 超过 GitHub 单文件限制，故使用高压缩率 7z 格式。

GPU 包包含 LibTorch 2.9.0 CUDA 12.8 和 cuDNN 9.10.2 运行库，
无需安装 CUDA Toolkit、Python、Qt 或编译器。需要兼容 CUDA 12.8 的
NVIDIA 显卡及驱动，建议更新到支持自己显卡的最新驱动。
默认通过运行检查后使用 `CUDA:0`，不可用时自动回退到 CPU。
训练日志可确认使用的设备；环境变量 `NNMTM_DEVICE=cpu` 可强制 CPU。
没有 NVIDIA 显卡建议下载体积更小的 CPU 包。

应用源码保持 v0.1.1 不变；额外的打包脚本和说明更新在 main 分支。
CUDA/cuDNN 为 NVIDIA 许可组件，不由项目 MIT 许可证覆盖；完整许可随包提供。

## 变化

- 保留训练曲线、回归散点图和召回率柱状图，移除 Qt Charts 依赖。
- 新增便携模式、启动入口、可重复打包脚本和 SHA256 校验。
- 包含第三方许可证及运行文件 SHA256 清单。
- 同一 Release 的 `Qt-6.10.1-sources.zip` 提供所用 Qt 模块的对应源码。
- CNN 的完整流程测试现在也覆盖 CPU。

## 验证与限制

CPU 构建验证核心逻辑、ANN/CNN 训练/保存/重载/推理、图表渲染及便携包启动。
包启动检查使用仅含 Windows 系统目录的 PATH，避免依赖开发机的 Qt/LibTorch 路径。
本次未在另一台全新 Windows 电脑上验证。

GPU 版另在本机 RTX 5060 上验证 CUDA 运算与参数更新、ANN/CNN 完整流程。
便携包检查隔离开发工具路径，同时检查强制 CPU 和隐藏 GPU 后的自动回退。
隐藏 GPU 是测试模拟，不代表已在没有 NVIDIA 驱动的电脑上验证。

程序为未签名的预览版；可使用随附 `.sha256` 文件核对下载内容。
项目代码使用 MIT License，依赖库分别遵循自己的许可证。
