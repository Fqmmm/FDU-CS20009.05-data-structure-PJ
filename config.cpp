#include "config.h"

// BPR 函数参数默认值
double BPRConfig::alpha = 0.15;
double BPRConfig::beta = 4.0;
double BPRConfig::lane_capacity = 1800.0;

// 缓存参数默认值
size_t CacheConfig::max_size = 50;
std::string CacheConfig::cache_dir = ".cache";
