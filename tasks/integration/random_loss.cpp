#include "random_loss.h"

#include <random>

double& GetProbability() {
    static double probability{0};
    return probability;
}

void EnableRandomLoss(double probability) {
    GetProbability() = probability;
}

bool ShouldBeLost() {
    static std::mt19937 gen{std::random_device{}()};
    static std::uniform_real_distribution<double> dist{0, 1};
    return dist(gen) < GetProbability();
}
