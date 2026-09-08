#pragma once
#include <torch/torch.h>

namespace nn = torch::nn;

// 模型 0: SimpleMLP
// 结构: Input -> [Linear->BN->ReLU] * 3 -> Linear(Output)
struct SimpleMLP : nn::Module {
    nn::Linear fc1{ nullptr }, fc2{ nullptr }, fc_out{ nullptr };
    nn::BatchNorm1d bn1{ nullptr }, bn2{ nullptr };

    SimpleMLP(int input_dim,
        int output_dim = 1,
        int layer1_dim = 16,
        int layer2_dim = 8) {

        layer1_dim = (double)input_dim * 2 / 3 + 1;
        layer2_dim = (layer1_dim + 1) / 2;

        // Hidden Layer 1
        fc1 = register_module("fc1", nn::Linear(input_dim, layer1_dim));
        bn1 = register_module("bn1", nn::BatchNorm1d(layer1_dim));

        // Hidden Layer 2
        fc2 = register_module("fc2", nn::Linear(layer1_dim, layer2_dim));
        bn2 = register_module("bn2", nn::BatchNorm1d(layer2_dim));

        // Output Layer
        fc_out = register_module("fc_out", nn::Linear(layer2_dim, output_dim));
    }

    torch::Tensor forward(torch::Tensor x) {
        x = torch::relu(bn1(fc1(x)));
        x = torch::relu(bn2(fc2(x)));
        x = fc_out(x); // Linear activation
        return x;
    }
};


// 模型 1: GeneralMLP（同时也是 L2RegularizedMLP 的结构）
// 结构: Input -> [Linear->BN->ReLU] * 3 -> Linear(Output)
struct GeneralMLP : nn::Module {
    nn::Linear fc1{ nullptr }, fc2{ nullptr }, fc3{ nullptr }, fc_out{ nullptr };
    nn::BatchNorm1d bn1{ nullptr }, bn2{ nullptr }, bn3{ nullptr };

    GeneralMLP(int input_dim, 
               int output_dim = 1, 
               int layer1_dim = 128, 
               int layer2_dim = 64, 
               int layer3_dim = 32) {

        layer1_dim = (double)input_dim * 2 / 3 + 1;
        layer2_dim = (layer1_dim + 1) / 2;
        layer3_dim = (layer2_dim + 1) / 2;

        // Hidden Layer 1
        fc1 = register_module("fc1", nn::Linear(input_dim, layer1_dim));
        bn1 = register_module("bn1", nn::BatchNorm1d(layer1_dim));

        // Hidden Layer 2
        fc2 = register_module("fc2", nn::Linear(layer1_dim, layer2_dim));
        bn2 = register_module("bn2", nn::BatchNorm1d(layer2_dim));

        // Hidden Layer 3
        fc3 = register_module("fc3", nn::Linear(layer2_dim, layer3_dim));
        bn3 = register_module("bn3", nn::BatchNorm1d(layer3_dim));

        // Output Layer
        fc_out = register_module("fc_out", nn::Linear(layer3_dim, output_dim));
    }

    torch::Tensor forward(torch::Tensor x) {
        x = torch::relu(bn1(fc1(x)));
        x = torch::relu(bn2(fc2(x)));
        x = torch::relu(bn3(fc3(x)));
        x = fc_out(x); // Linear activation
        return x;
    }
};

// 模型 2: DropoutMLP
// 结构: Input -> [Linear->BN->ReLU->Dropout] * 3 -> Linear(Output)
struct DropoutMLP : nn::Module {
    nn::Linear fc1{ nullptr }, fc2{ nullptr }, fc3{ nullptr }, fc_out{ nullptr };
    nn::BatchNorm1d bn1{ nullptr }, bn2{ nullptr }, bn3{ nullptr };
    nn::Dropout dropout{ nullptr };

    DropoutMLP(int input_dim,
               int output_dim = 1, 
               int layer1_dim = 128, 
               int layer2_dim = 64, 
               int layer3_dim = 32) {
        
        layer1_dim = (double)input_dim * 2 / 3 + 1;
        layer2_dim = (layer1_dim + 1) / 2;
        layer3_dim = (layer2_dim + 1) / 2;

        // Hidden Layer 1
        fc1 = register_module("fc1", nn::Linear(input_dim, layer1_dim));
        bn1 = register_module("bn1", nn::BatchNorm1d(layer1_dim));

        // Hidden Layer 2
        fc2 = register_module("fc2", nn::Linear(layer1_dim, layer2_dim));
        bn2 = register_module("bn2", nn::BatchNorm1d(layer2_dim));

        // Hidden Layer 3
        fc3 = register_module("fc3", nn::Linear(layer2_dim, layer3_dim));
        bn3 = register_module("bn3", nn::BatchNorm1d(layer3_dim));

        // Output Layer
        fc_out = register_module("fc_out", nn::Linear(layer3_dim, output_dim));

        // Dropout (p=0.3 per document)
        dropout = register_module("dropout", nn::Dropout(0.3));
    }

    torch::Tensor forward(torch::Tensor x) {
        x = dropout(torch::relu(bn1(fc1(x))));
        x = dropout(torch::relu(bn2(fc2(x))));
        x = dropout(torch::relu(bn3(fc3(x))));
        x = fc_out(x);
        return x;
    }
};

// 模型 3: L2RegularizedMLP
// 结构: 与 GeneralMLP 相同，此处不再重复书写
// 注意: L2 Regularization (Weight Decay) 在 Optimizer 中设置。
// 例如: torch::optim::Adam(model.parameters(), torch::optim::AdamOptions(lr).weight_decay(0.001));


// 模型 4: DeepMLP_Regression
// 结构: Input -> [Linear->BN->ReLU] * 5 -> Linear(Output)
struct DeepMLP : nn::Module {
    nn::Linear fc1{ nullptr }, fc2{ nullptr }, fc3{ nullptr }, fc4{ nullptr }, fc5{ nullptr }, fc_out{ nullptr };
    nn::BatchNorm1d bn1{ nullptr }, bn2{ nullptr }, bn3{ nullptr }, bn4{ nullptr }, bn5{ nullptr };

    DeepMLP(int input_dim,
        int output_dim = 1,
        int layer1_dim = 256, 
        int layer2_dim = 128,
        int layer3_dim = 64,
        int layer4_dim = 32,
        int layer5_dim = 16) {

        // Layer 1
        fc1 = register_module("fc1", nn::Linear(input_dim, layer1_dim));
        bn1 = register_module("bn1", nn::BatchNorm1d(layer1_dim));

        // Layer 2
        fc2 = register_module("fc2", nn::Linear(layer1_dim, layer2_dim));
        bn2 = register_module("bn2", nn::BatchNorm1d(layer2_dim));

        // Layer 3
        fc3 = register_module("fc3", nn::Linear(layer2_dim, layer3_dim));
        bn3 = register_module("bn3", nn::BatchNorm1d(layer3_dim));

        // Layer 4
        fc4 = register_module("fc4", nn::Linear(layer3_dim, layer4_dim));
        bn4 = register_module("bn4", nn::BatchNorm1d(layer4_dim));

        // Layer 5
        fc5 = register_module("fc5", nn::Linear(layer4_dim, layer5_dim));
        bn5 = register_module("bn5", nn::BatchNorm1d(layer5_dim));

        // Output Layer
        fc_out = register_module("fc_out", nn::Linear(layer5_dim, output_dim));
    }

    torch::Tensor forward(torch::Tensor x) {
        x = torch::relu(bn1(fc1(x)));
        x = torch::relu(bn2(fc2(x)));
        x = torch::relu(bn3(fc3(x)));
        x = torch::relu(bn4(fc4(x)));
        x = torch::relu(bn5(fc5(x)));
        x = fc_out(x);
        return x;
    }
};


// 模型 5: Super_DeepMLP_Regression
struct SuperDeepMLP : nn::Module {
    nn::Linear fc1{ nullptr }, fc2{ nullptr }, fc3{ nullptr }, fc4{ nullptr }, fc5{ nullptr }, fc6{ nullptr }, fc7{ nullptr }, fc_out{ nullptr };
    nn::BatchNorm1d bn1{ nullptr }, bn2{ nullptr }, bn3{ nullptr }, bn4{ nullptr }, bn5{ nullptr }, bn6{ nullptr }, bn7{ nullptr };

    SuperDeepMLP(int input_dim,
        int output_dim = 1,
        int layer1_dim = 1024,
        int layer2_dim = 512,
        int layer3_dim = 256,
        int layer4_dim = 128,
        int layer5_dim = 64,
        int layer6_dim = 32,
        int layer7_dim = 16
    ) {

        // Layer 1
        fc1 = register_module("fc1", nn::Linear(input_dim, layer1_dim));
        bn1 = register_module("bn1", nn::BatchNorm1d(layer1_dim));

        // Layer 2
        fc2 = register_module("fc2", nn::Linear(layer1_dim, layer2_dim));
        bn2 = register_module("bn2", nn::BatchNorm1d(layer2_dim));

        // Layer 3
        fc3 = register_module("fc3", nn::Linear(layer2_dim, layer3_dim));
        bn3 = register_module("bn3", nn::BatchNorm1d(layer3_dim));

        // Layer 4
        fc4 = register_module("fc4", nn::Linear(layer3_dim, layer4_dim));
        bn4 = register_module("bn4", nn::BatchNorm1d(layer4_dim));

        // Layer 5
        fc5 = register_module("fc5", nn::Linear(layer4_dim, layer5_dim));
        bn5 = register_module("bn5", nn::BatchNorm1d(layer5_dim));

        // Layer 6
        fc6 = register_module("fc6", nn::Linear(layer5_dim, layer6_dim));
        bn6 = register_module("bn6", nn::BatchNorm1d(layer6_dim));

        // Layer 7
        fc7 = register_module("fc7", nn::Linear(layer6_dim, layer7_dim));
        bn7 = register_module("bn7", nn::BatchNorm1d(layer7_dim));

        // Output Layer
        fc_out = register_module("fc_out", nn::Linear(layer7_dim, output_dim));
    }

    torch::Tensor forward(torch::Tensor x) {
        x = torch::relu(bn1(fc1(x)));
        x = torch::relu(bn2(fc2(x)));
        x = torch::relu(bn3(fc3(x)));
        x = torch::relu(bn4(fc4(x)));
        x = torch::relu(bn5(fc5(x)));
        x = torch::relu(bn6(fc6(x)));
        x = torch::relu(bn7(fc7(x)));
        x = fc_out(x);
        return x;
    }
};
