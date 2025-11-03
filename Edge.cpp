#include "Edge.h"
#include "config.h"
#include <cmath>

Edge::Edge(const std::string& dest, double len, double spd_limit, int num_lanes, int vehicles)
    : destination(dest),
      length(len),
      speed_limit(spd_limit),
      lanes(num_lanes),
      current_vehicles(vehicles)
{
    weight = calculate_weight(); 
}

// 使用BPR函数计算单条边的权重
double Edge::calculate_weight() const
{
    // 检查车道数或限速是否为0，避免除零错误
    if (lanes <= 0 || speed_limit <= 0)
    {
        return std::numeric_limits<double>::infinity();
    }

    // 计算自由流通行时间（无拥堵时的理论通行时间）
    double speed_mps = speed_limit * 1000.0 / 3600.0;   // 转换为 m/s
    double free_flow_time = length / speed_mps;         // 秒

    // 计算道路容量（每小时）
    double capacity = lanes * BPRConfig::lane_capacity;

    // 计算流量/容量比
    double volume_capacity_ratio = static_cast<double>(current_vehicles) / capacity;

    // BPR 函数：T = T₀ × [1 + α × (V/C)^β]
    double congestion_multiplier = 1.0 + BPRConfig::alpha * std::pow(volume_capacity_ratio, BPRConfig::beta);
    return free_flow_time * congestion_multiplier;
}
