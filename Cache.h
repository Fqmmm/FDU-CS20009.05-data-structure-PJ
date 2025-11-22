#ifndef CACHE_H
#define CACHE_H

#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <filesystem>
#include <chrono>
#include "Graph.h"  // 引入PathResult和MultiPath

// 文件签名：使用修改时间和文件大小作为轻量级的文件变化检测
struct FileSignature
{
    std::string path;
    std::filesystem::file_time_type mtime;
    uintmax_t size;

    FileSignature() : path(""), mtime(), size(0) {}

    FileSignature(const std::string &file_path);

    // 检查文件是否仍然匹配当前签名（文件未被修改）
    bool matches(const std::string &file_path) const;

    // 将签名转换为字符串（用于生成缓存键）
    std::string to_string() const;
};

// 缓存条目
struct CacheEntry
{
    std::string start;
    std::string end;
    FileSignature csv_signature;
    std::string cache_file; // 缓存文件路径
    std::chrono::system_clock::time_point created_at;

    CacheEntry() : start(""), end(""), csv_signature(), cache_file(""), created_at() {}
};

// LRU缓存类
class PathCache
{
public:
    // 构造函数
    // cache_dir: 缓存目录路径
    // max_size: LRU缓存最大条目数
    PathCache(const std::string &cache_dir = ".cache", size_t max_size = 50);

    // 查询缓存，返回MultiPath，如果未命中则所有路径为空
    MultiPath get(const std::string &start,
                  const std::string &end,
                  const std::string &csv_file);

    // 保存到缓存（三种路径一起保存）
    void put(const std::string &start,
             const std::string &end,
             const std::string &csv_file,
             const MultiPath &paths);

    // 清空所有缓存
    void clear();

    // 获取缓存统计信息
    size_t get_hit_count() const { return hit_count; }
    size_t get_miss_count() const { return miss_count; }
    size_t get_entry_count() const { return entries.size(); }

private:
    std::string cache_dir;
    std::string paths_dir;       // cache_dir/paths/
    std::string index_file_path; // cache_dir/cache_index.txt
    size_t max_size;

    // LRU数据结构
    std::list<std::string> lru_list;                      // 最近使用顺序，前面是最近使用的
    std::unordered_map<std::string, CacheEntry> entries; // 键 -> 缓存条目

    // 统计信息
    size_t hit_count;
    size_t miss_count;

    // 初始化缓存目录
    void init_cache_dir();

    // 加载索引文件
    void load_index();

    // 保存索引文件
    void save_index();

    // 淘汰最久未使用的缓存条目
    void evict_lru();

    // 更新LRU顺序（将键移到最前面）
    void touch(const std::string &key);

    // 生成缓存键
    std::string generate_key(const std::string &start,
                             const std::string &end,
                             const FileSignature &sig);

    // 从缓存文件读取多路径
    MultiPath read_cache_file(const std::string &file_path);

    // 写入多路径到缓存文件
    void write_cache_file(const std::string &file_path, const MultiPath &paths);
};

#endif
