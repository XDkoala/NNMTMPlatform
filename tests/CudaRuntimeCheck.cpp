#include <torch/torch.h>

#include <cmath>
#include <iostream>

int main() {
    if (!torch::cuda::is_available()) {
        std::cout << "CUDA is not available; skipping GPU runtime check.\n";
        return 77;
    }

    try {
        const torch::Device device(torch::kCUDA, 0);
        auto left = torch::full({256, 256}, 2.0f,
            torch::TensorOptions().dtype(torch::kFloat32).device(device));
        auto right = torch::full({256, 256}, 3.0f,
            torch::TensorOptions().dtype(torch::kFloat32).device(device));
        auto result = torch::matmul(left, right);
        const float value = result.index({0, 0}).item<float>();
        if (std::abs(value - 1536.0f) > 0.01f) {
            std::cerr << "Unexpected CUDA result: " << value << '\n';
            return 1;
        }

        auto model = torch::nn::Sequential(
            torch::nn::Linear(8, 16),
            torch::nn::ReLU(),
            torch::nn::Linear(16, 1));
        model->to(device);
        auto input = torch::randn({64, 8},
            torch::TensorOptions().dtype(torch::kFloat32).device(device));
        auto target = torch::randn({64, 1},
            torch::TensorOptions().dtype(torch::kFloat32).device(device));
        torch::optim::Adam optimizer(model->parameters(), torch::optim::AdamOptions(1e-3));
        optimizer.zero_grad();
        auto loss = torch::mse_loss(model->forward(input), target);
        loss.backward();
        optimizer.step();
        if (!std::isfinite(loss.item<float>())) {
            std::cerr << "CUDA training step returned a non-finite loss.\n";
            return 1;
        }
        for (const auto& parameter : model->parameters()) {
            if (!parameter.device().is_cuda()) {
                std::cerr << "A model parameter remained on CPU.\n";
                return 1;
            }
        }

        std::cout << "CUDA matrix and training checks passed on CUDA:0.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "CUDA runtime check failed: " << error.what() << '\n';
        return 1;
    }
}
