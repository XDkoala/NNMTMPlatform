# 自动化测试

- `CoreLogicTests.cpp`：不依赖 Qt/LibTorch 的数据划分与输入校验。
- `CudaRuntimeCheck.cpp`：验证 LibTorch 能执行真实 CUDA 运算和 Adam 更新。
- `AnnPipelineCheck.cpp`：验证 ANN 的训练、保存、重载和分批推理。
- `CnnPipelineCheck.cpp`：验证 CNN 图片分批读取、CUDA 训练、保存、重载和推理。

构建完成后在项目根目录运行：

```powershell
ctest --test-dir build/local-windows-release --output-on-failure
```

未检测到 CUDA 时，CUDA 专用测试会显示为跳过，而不是失败。测试创建的模型与数据会在结束时自动删除。
