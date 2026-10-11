# NNMTMPlatform v0.1.0

首次源码预览版：将大一课程设计整理为可通过 CMake 构建的本地神经网络训练与模型管理工具。

## 功能

- ANN 数值回归、二分类和多分类，CNN 图像二分类和多分类。
- 模型训练、测试、推理，以及模型搜索、筛选、草稿箱和回收站。
- 本地保存模型权重、元数据、归一化参数及标签映射。
- 训练损失曲线、回归和分类结果分析。
- 可配置训练轮数、批次大小和学习率。
- CUDA 运算检查、自动 GPU 加速和 CPU 回退。
- CNN 分批图片读取。

## 工程与验证

- C++17、Qt 6、LibTorch、OpenCV，使用 CMake/Ninja 构建。
- 提供 Windows 构建指南、数据格式说明和数值示例。
- 本地验证覆盖核心逻辑、CUDA 运行时、ANN 和 CNN 的训练/保存/重载/推理流程。
- GitHub Actions 自动检查核心逻辑。
- MIT License。

## 下载与使用

本次发布提供源码。下载下方的 **Source code (zip)** 或 **Source code (tar.gz)**，按仓库 README 和 `docs/BUILDING.zh-CN.md` 配置依赖并构建。

## 当前限制

- 这是用于学习和作品展示的预览版本。
- 完整程序主要在 Windows + MSVC 环境验证。
- CUDA 加速需要兼容的 NVIDIA 显卡、驱动和 CUDA 版 LibTorch。
- GitHub Actions 目前仅覆盖核心逻辑，完整 Qt/LibTorch 流程在本机验证。
- 此次发布未提供预编译 Windows 程序包。
