#ifndef CONFIG_H
#define CONFIG_H

#include <string>

// 路径权重模式
enum class WeightMode
{
    TIME,       // 时间最短（使用BPR拥堵模型）
    DISTANCE,   // 距离最短（使用道路长度）
    BALANCED    // 综合推荐（时间和距离的归一化加权平均）
};

// BPR 函数配置参数
struct BPRConfig
{
    static double alpha;            // 拥堵敏感系数，默认 0.15
    static double beta;             // 拥堵指数，默认 4.0
    static double lane_capacity;    // 每车道每小时通行能力，默认 1800 辆
};

// 缓存配置参数
struct CacheConfig
{
    static size_t max_size;         // LRU 缓存最大条目数，默认 50
    static std::string cache_dir;   // 缓存目录路径，默认 ".cache"
};

// 综合路径权重配置参数
struct PathWeightConfig
{
    static double time_factor;       // 时间的权重因子（α），默认 0.6
    static double distance_factor;   // 距离的权重因子（1-α），默认 0.4
};

#endif // CONFIG_H
