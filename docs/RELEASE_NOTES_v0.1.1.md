# NNMTMPlatform v0.1.1

新增 Windows 可运行便携包，下载后无需编译。

## Windows 使用

1. 下载 `NNMTMPlatform-v0.1.1-windows-x64-cpu.zip`。
2. 完整解压到可写目录，双击 `Start.cmd` 或 `NNMTMPlatform.exe`。
3. 用 `examples/numeric/` 中的示例开始训练和推理。

运行库已包含，无需安装 Visual Studio、Qt、Python、LibTorch 或 CUDA。
系统要求：Windows 10/11 x64，支持 AVX2 的处理器。建议至少 8 GB 内存。
此包使用 CPU；GPU 加速仍可通过源码构建使用 CUDA 版 LibTorch。

模型数据保存在程序旁的 `data/`，更新或移动程序前请备份。

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

程序为未签名的预览版；可使用随附 `.sha256` 文件核对下载内容。
项目代码使用 MIT License，依赖库分别遵循自己的许可证。
