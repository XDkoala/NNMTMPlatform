# 示例数据

`numeric/` 中的文件可直接在 NNMTMPlatform 中使用：

- `binary_train.txt`：两个特征的二分类训练集。
- `binary_test.txt`：二分类测试集，包含真实标签。
- `binary_infer.txt`：二分类推理集，不含标签。
- `regression_train.txt`：两个特征的线性回归训练集。
- `regression_infer.txt`：回归推理集。

快速试用二分类：

1. 新建“数值二分类”模型。
2. 输入特征数填 `2`，架构可选 `SimpleMLP`。
3. 训练文件选择 `binary_train.txt`。
4. 训练完成后，测试选择 `binary_test.txt`，推理选择 `binary_infer.txt`。

这些数据只用于验证流程，不用于评估模型质量。图像数据的目录布局见 [数据格式说明](../docs/DATA_FORMAT.zh-CN.md)。
