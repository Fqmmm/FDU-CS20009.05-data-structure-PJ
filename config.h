#ifndef CONFIG_H
#define CONFIG_H

// BPR 函数配置参数
struct BPRConfig
{
    static double alpha;            // 拥堵敏感系数，默认 0.15
    static double beta;             // 拥堵指数，默认 4.0
    static double lane_capacity;    // 每车道每小时通行能力，默认 1800 辆
};

#endif // CONFIG_H
