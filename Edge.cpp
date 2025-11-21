#include "Edge.h"

Edge::Edge(const std::string& dest, double len, double spd_limit, int num_lanes, int vehicles)
    : destination(dest),
      length(len),
      speed_limit(spd_limit),
      lanes(num_lanes),
      current_vehicles(vehicles),
      time(0.0),
      balanced_score(0.0)
{
    // time和balanced_score将在Graph::from_csv()中计算
}

// 根据权重模式获取边的权重
double Edge::get_weight(WeightMode mode) const
{
    switch (mode)
    {
    case WeightMode::TIME:
        // 时间最短：使用预计算的时间
        return time;

    case WeightMode::DISTANCE:
        // 距离最短：使用道路长度
        return length;

    case WeightMode::BALANCED:
        // 综合推荐：使用预计算的综合评分
        return balanced_score;

    default:
        return time;
    }
}
