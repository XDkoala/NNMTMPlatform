#pragma once
#include <torch/torch.h>

namespace nn = torch::nn;

// ---------------------------------------------------------
// 辅助函数：创建一个 3x3 卷积层 (用于 ResNet)
// ---------------------------------------------------------
inline nn::Conv2d conv3x3(int64_t in_planes, int64_t out_planes, int64_t stride = 1, int64_t groups = 1, int64_t dilation = 1) {
    return nn::Conv2d(nn::Conv2dOptions(in_planes, out_planes, 3)
        .stride(stride)
        .padding(dilation)
        .groups(groups)
        .bias(false)
        .dilation(dilation));
}

// 辅助函数：创建一个 1x1 卷积层 (用于 ResNet 下采样)
inline nn::Conv2d conv1x1(int64_t in_planes, int64_t out_planes, int64_t stride = 1) {
    return nn::Conv2d(nn::Conv2dOptions(in_planes, out_planes, 1)
        .stride(stride)
        .bias(false));
}

// ---------------------------------------------------------
// 模型 1: LeNet (适配版)
// 原版 LeNet-5 是针对 32x32 输入设计的。
// 为了适配 224x224 输入，我们在最后增加了自适应池化层。
// ---------------------------------------------------------
struct LeNet : nn::Module {
    nn::Conv2d conv1{ nullptr }, conv2{ nullptr };
    nn::Linear fc1{ nullptr }, fc2{ nullptr }, fc3{ nullptr };

    explicit LeNet(int64_t num_classes) {
        // Input: 3 x 224 x 224
        // Conv1: 3 -> 6, Kernel 5. Output: 6 x 220 x 220 -> Pool -> 6 x 110 x 110
        conv1 = register_module("conv1", nn::Conv2d(nn::Conv2dOptions(3, 6, 5)));

        // Conv2: 6 -> 16, Kernel 5. Output: 16 x 106 x 106 -> Pool -> 16 x 53 x 53
        conv2 = register_module("conv2", nn::Conv2d(nn::Conv2dOptions(6, 16, 5)));

        // Fully Connected Layers
        // 使用 16 * 53 * 53 会导致参数过大，通常 LeNet 不适合处理这么大的图。
        // 但为了演示，我们保持结构。更好的做法是在 fc1 前加 AdaptiveAvgPool2d((5,5))
        // 这样展平后就是 16 * 5 * 5 = 400，回归经典 LeNet 规模

        fc1 = register_module("fc1", nn::Linear(16 * 5 * 5, 120));
        fc2 = register_module("fc2", nn::Linear(120, 84));
        fc3 = register_module("fc3", nn::Linear(84, num_classes));
    }

    torch::Tensor forward(torch::Tensor x) {
        // Conv 1
        x = torch::max_pool2d(torch::relu(conv1(x)), 2);
        // Conv 2
        x = torch::max_pool2d(torch::relu(conv2(x)), 2);

        // 强行将特征图池化到 5x5 大小，以便衔接全连接层，避免显存爆炸
        x = torch::adaptive_avg_pool2d(x, { 5, 5 });

        // Flatten
        x = x.view({ x.size(0), -1 });

        // FC
        x = torch::relu(fc1(x));
        x = torch::relu(fc2(x));
        x = fc3(x);
        return x;
    }
};

// ---------------------------------------------------------
// 模型 2: AlexNet
// 经典的深度卷积网络，包含 5 层卷积和 3 层全连接
// ---------------------------------------------------------
struct AlexNet : nn::Module {
    nn::Sequential features{ nullptr };
    nn::Sequential classifier{ nullptr };

    explicit AlexNet(int64_t num_classes) {
        // 特征提取部分
        features = register_module("features", nn::Sequential(
            // Conv 1
            nn::Conv2d(nn::Conv2dOptions(3, 64, 11).stride(4).padding(2)),
            nn::ReLU(nn::ReLUOptions().inplace(true)),
            nn::MaxPool2d(nn::MaxPool2dOptions(3).stride(2)),

            // Conv 2
            nn::Conv2d(nn::Conv2dOptions(64, 192, 5).padding(2)),
            nn::ReLU(nn::ReLUOptions().inplace(true)),
            nn::MaxPool2d(nn::MaxPool2dOptions(3).stride(2)),

            // Conv 3
            nn::Conv2d(nn::Conv2dOptions(192, 384, 3).padding(1)),
            nn::ReLU(nn::ReLUOptions().inplace(true)),

            // Conv 4
            nn::Conv2d(nn::Conv2dOptions(384, 256, 3).padding(1)),
            nn::ReLU(nn::ReLUOptions().inplace(true)),

            // Conv 5
            nn::Conv2d(nn::Conv2dOptions(256, 256, 3).padding(1)),
            nn::ReLU(nn::ReLUOptions().inplace(true)),
            nn::MaxPool2d(nn::MaxPool2dOptions(3).stride(2))
        ));

        // 分类器部分
        classifier = register_module("classifier", nn::Sequential(
            nn::Dropout(0.5),
            nn::Linear(256 * 6 * 6, 4096),
            nn::ReLU(nn::ReLUOptions().inplace(true)),

            nn::Dropout(0.5),
            nn::Linear(4096, 4096),
            nn::ReLU(nn::ReLUOptions().inplace(true)),

            nn::Linear(4096, num_classes)
        ));
    }

    torch::Tensor forward(torch::Tensor x) {
        x = features->forward(x);
        // 自适应池化确保输出是 6x6，匹配 Linear 的输入 (256*6*6)
        x = torch::adaptive_avg_pool2d(x, { 6, 6 });
        x = x.view({ x.size(0), -1 });
        x = classifier->forward(x);
        return x;
    }
};

// ---------------------------------------------------------
// 模型 3: VGGNet (VGG-16)
// 使用大量 3x3 小卷积核堆叠
// ---------------------------------------------------------
struct VGGNet : nn::Module {
    nn::Sequential features{ nullptr };
    nn::Sequential classifier{ nullptr };

    explicit VGGNet(int64_t num_classes) {
        features = register_module("features", nn::Sequential(
            // Block 1
            nn::Conv2d(nn::Conv2dOptions(3, 64, 3).padding(1)), nn::BatchNorm2d(64), nn::ReLU(true),
            nn::Conv2d(nn::Conv2dOptions(64, 64, 3).padding(1)), nn::BatchNorm2d(64), nn::ReLU(true),
            nn::MaxPool2d(nn::MaxPool2dOptions(2).stride(2)),

            // Block 2
            nn::Conv2d(nn::Conv2dOptions(64, 128, 3).padding(1)), nn::BatchNorm2d(128), nn::ReLU(true),
            nn::Conv2d(nn::Conv2dOptions(128, 128, 3).padding(1)), nn::BatchNorm2d(128), nn::ReLU(true),
            nn::MaxPool2d(nn::MaxPool2dOptions(2).stride(2)),

            // Block 3
            nn::Conv2d(nn::Conv2dOptions(128, 256, 3).padding(1)), nn::BatchNorm2d(256), nn::ReLU(true),
            nn::Conv2d(nn::Conv2dOptions(256, 256, 3).padding(1)), nn::BatchNorm2d(256), nn::ReLU(true),
            nn::Conv2d(nn::Conv2dOptions(256, 256, 3).padding(1)), nn::BatchNorm2d(256), nn::ReLU(true),
            nn::MaxPool2d(nn::MaxPool2dOptions(2).stride(2)),

            // Block 4
            nn::Conv2d(nn::Conv2dOptions(256, 512, 3).padding(1)), nn::BatchNorm2d(512), nn::ReLU(true),
            nn::Conv2d(nn::Conv2dOptions(512, 512, 3).padding(1)), nn::BatchNorm2d(512), nn::ReLU(true),
            nn::Conv2d(nn::Conv2dOptions(512, 512, 3).padding(1)), nn::BatchNorm2d(512), nn::ReLU(true),
            nn::MaxPool2d(nn::MaxPool2dOptions(2).stride(2)),

            // Block 5
            nn::Conv2d(nn::Conv2dOptions(512, 512, 3).padding(1)), nn::BatchNorm2d(512), nn::ReLU(true),
            nn::Conv2d(nn::Conv2dOptions(512, 512, 3).padding(1)), nn::BatchNorm2d(512), nn::ReLU(true),
            nn::Conv2d(nn::Conv2dOptions(512, 512, 3).padding(1)), nn::BatchNorm2d(512), nn::ReLU(true),
            nn::MaxPool2d(nn::MaxPool2dOptions(2).stride(2))
        ));

        classifier = register_module("classifier", nn::Sequential(
            nn::Linear(512 * 7 * 7, 4096),
            nn::ReLU(true),
            nn::Dropout(0.5),
            nn::Linear(4096, 4096),
            nn::ReLU(true),
            nn::Dropout(0.5),
            nn::Linear(4096, num_classes)
        ));
    }

    torch::Tensor forward(torch::Tensor x) {
        x = features->forward(x);
        x = torch::adaptive_avg_pool2d(x, { 7, 7 });
        x = x.view({ x.size(0), -1 });
        x = classifier->forward(x);
        return x;
    }
};

// ---------------------------------------------------------
// 模型 4: ResNet (ResNet-18)
// 使用残差块 (Residual Block)
// ---------------------------------------------------------

// ResNet 的基础残差块
struct BasicBlock : nn::Module {
    nn::Conv2d conv1{ nullptr }, conv2{ nullptr };
    nn::BatchNorm2d bn1{ nullptr }, bn2{ nullptr };
    nn::Sequential downsample{ nullptr };

    BasicBlock(int64_t inplanes, int64_t planes, int64_t stride = 1) {
        conv1 = register_module("conv1", conv3x3(inplanes, planes, stride));
        bn1 = register_module("bn1", nn::BatchNorm2d(planes));
        conv2 = register_module("conv2", conv3x3(planes, planes));
        bn2 = register_module("bn2", nn::BatchNorm2d(planes));

        // 如果步长不为1或者输入输出通道数不一致，需要下采样调整维度，以便相加
        if (stride != 1 || inplanes != planes) {
            downsample = register_module("downsample", nn::Sequential(
                conv1x1(inplanes, planes, stride),
                nn::BatchNorm2d(planes)
            ));
        }
    }

    torch::Tensor forward(torch::Tensor x) {
        auto identity = x;

        auto out = conv1(x);
        out = bn1(out);
        out = torch::relu(out);

        out = conv2(out);
        out = bn2(out);

        // Shortcut connection
        if (!downsample.is_empty()) {
            identity = downsample->forward(x);
        }

        out += identity;
        out = torch::relu(out);

        return out;
    }
};

// ResNet-18 主体
struct ResNet : nn::Module {
    int64_t inplanes = 64;
    nn::Conv2d conv1{ nullptr };
    nn::BatchNorm2d bn1{ nullptr };
    nn::Sequential layer1{ nullptr }, layer2{ nullptr }, layer3{ nullptr }, layer4{ nullptr };
    nn::Linear fc{ nullptr };

    explicit ResNet(int64_t num_classes) {
        // Initial Conv: 7x7, stride 2
        conv1 = register_module("conv1", nn::Conv2d(nn::Conv2dOptions(3, 64, 7).stride(2).padding(3).bias(false)));
        bn1 = register_module("bn1", nn::BatchNorm2d(64));

        // Layers (ResNet-18 has blocks structure: 2, 2, 2, 2)
        layer1 = register_module("layer1", make_layer(64, 2));
        layer2 = register_module("layer2", make_layer(128, 2, 2)); // stride 2
        layer3 = register_module("layer3", make_layer(256, 2, 2)); // stride 2
        layer4 = register_module("layer4", make_layer(512, 2, 2)); // stride 2

        // Output Layer
        fc = register_module("fc", nn::Linear(512, num_classes));
    }

    // 辅助函数：构建包含多个 BasicBlock 的层
    nn::Sequential make_layer(int64_t planes, int64_t blocks, int64_t stride = 1) {
        nn::Sequential layers;
        // 第一个 block 负责改变 stride (降采样)
        layers->push_back(BasicBlock(inplanes, planes, stride));
        inplanes = planes;

        // 后续 block stride 均为 1
        for (int64_t i = 1; i < blocks; ++i) {
            layers->push_back(BasicBlock(inplanes, planes));
        }
        return layers;
    }

    torch::Tensor forward(torch::Tensor x) {
        // Stem
        x = conv1(x);
        x = bn1(x);
        x = torch::relu(x);
        x = torch::max_pool2d(x, 3, 2, 1);

        // Layers
        x = layer1->forward(x);
        x = layer2->forward(x);
        x = layer3->forward(x);
        x = layer4->forward(x);

        // Global Average Pooling
        x = torch::adaptive_avg_pool2d(x, { 1, 1 });
        x = x.view({ x.size(0), -1 });

        // FC
        x = fc(x);
        return x;
    }
};