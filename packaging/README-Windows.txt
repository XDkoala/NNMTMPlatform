NNMTMPlatform Windows x64 CPU portable edition

使用方法
1. 将整个 ZIP 解压到可写目录，例如桌面上的 NNMTMPlatform 文件夹。
2. 双击 Start.cmd 或 NNMTMPlatform.exe。
3. 首次试用可选择 examples/numeric/binary_train.txt。
   新建数值二分类模型，输入特征数填 2，架构选 SimpleMLP。
   训练后用 binary_test.txt 测试，或用 binary_infer.txt 推理。

不需要安装 Visual Studio、Qt、Python、LibTorch 或 CUDA，也不需要自行编译。
系统要求：Windows 10/11 x64，支持 AVX2 的 x64 处理器。
这是 CPU 版，训练与推理使用 CPU，不能使用 GPU 加速。
建议至少 8 GB 内存。大型 CNN 模型需要更多内存，首次试用建议使用 LeNet。

请完整解压后启动，不要在压缩包内直接打开，也不要只移动 EXE。
portable.ini 使模型数据始终保存在程序旁边的 data/ 目录。
迁移或备份时请保留整个 data/；替换版本前先备份 data/。
不要放在 Program Files 等受保护、不可写目录。

这是未签名的开源预览程序。如 Windows 显示安全提示，请先核对下载地址
和对应的 SHA256 校验文件，再自行决定是否运行。

程序源码：https://github.com/XDkoala/NNMTMPlatform
问题反馈：https://github.com/XDkoala/NNMTMPlatform/issues
项目代码使用 MIT License；随附依赖遵循各自许可证，见 licenses/。
Qt 以动态 DLL 链接，可替换为兼容版本。对应 Qt 源码见同一 Release
的 Qt-6.10.1-sources.zip 及 THIRD_PARTY_NOTICES.md。
