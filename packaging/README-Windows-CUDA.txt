NNMTMPlatform Windows x64 CUDA 12.8 portable edition

用 7-Zip 或支持 7z 的工具完整解压到可写目录，双击 Start.cmd 或 NNMTMPlatform.exe。
可用 examples/numeric/binary_train.txt 创建二分类 SimpleMLP，输入特征数填 2。

系统要求：Windows 10/11 x64、支持 AVX2 的处理器、兼容 CUDA 12.8 的
NVIDIA 显卡及驱动。建议更新到支持自己显卡的最新驱动。
无需安装 Visual Studio、Qt、Python、LibTorch 或 CUDA Toolkit。
CUDA/cuDNN 运行库已包含，NVIDIA 显卡驱动不包含。

默认在 CUDA 运行检查成功后使用 GPU，否则回退到 CPU。
训练日志中的 Compute device: CUDA:0 表示正在使用 GPU。
如需强制使用 CPU，可在 PowerShell 中进入程序目录后运行：
  $env:NNMTM_DEVICE = 'cpu'
  .\NNMTMPlatform.exe
无 NVIDIA 显卡的使用者建议下载体积更小的 CPU 包。

GPU 包含大型运行库，解压需要约 5 GB 空间；大型 CNN 还需要足够显存和内存。
请不要仅移动 EXE 或在压缩包中直接启动。
模型保存在程序旁的 data/，升级前请备份整个 data/。
这是未签名的预览程序，可用随附 SHA256 校验文件核对下载内容。

项目源码使用 MIT License；第三方运行库分别遵循各自许可证。
NVIDIA 组件仅随本应用提供，不得将其单独分发或视为 MIT 开源组件。
许可证见 licenses/ 和 THIRD_PARTY_NOTICES.md。
对应 Qt 源码见同一 Release 的 Qt-6.10.1-sources.zip。
程序源码与问题反馈：https://github.com/XDkoala/NNMTMPlatform
