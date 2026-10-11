# 自动化测试

- `CoreLogicTests.cpp`：不依赖 Qt/LibTorch 的数据划分与输入校验。
- `CudaRuntimeCheck.cpp`：验证 LibTorch 能执行真实 CUDA 运算和 Adam 更新。
- `AnnPipelineCheck.cpp`：验证 ANN 的训练、保存、重载和分批推理。
- `CnnPipelineCheck.cpp`：验证 CNN 图片分批读取、CPU/CUDA 训练、保存、重载和推理。
- `PlotRenderCheck.cpp`：渲染折线、散点、柱状图、恒定值及空图表，输出 PNG 供视觉检查。

构建完成后在项目根目录运行：

```powershell
ctest --test-dir build/local-windows-release --output-on-failure
```

未检测到 CUDA 时，CUDA 专用测试会显示为跳过，而不是失败；CNN 完整流程仍会使用 CPU 测试。模型与数据测试的临时文件会自动删除，图表 PNG 保留在构建目录的 `plot-checks/`。
