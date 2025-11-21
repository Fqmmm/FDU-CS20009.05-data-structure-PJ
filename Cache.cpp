#include "Cache.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <functional>

// ============================================================================
// FileSignature 实现
// ============================================================================

FileSignature::FileSignature(const std::string &file_path)
{
    try
    {
        if (std::filesystem::exists(file_path))
        {
            // 规范化路径：将相对路径转换为绝对路径
            // 这样 "./foo" 和 "D:/bar/foo" 如果指向同一文件，会生成相同的签名
            path = std::filesystem::canonical(file_path).string();
            mtime = std::filesystem::last_write_time(file_path);
            size = std::filesystem::file_size(file_path);
        }
        else
        {
            // 文件不存在，使用原始路径
            path = file_path;
            size = 0;
        }
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        std::cerr << "Warning: Could not get file signature for " << file_path << ": " << e.what() << std::endl;
        path = file_path;
        size = 0;
    }
}

bool FileSignature::matches(const std::string &file_path) const
{
    try
    {
        if (!std::filesystem::exists(file_path))
        {
            return false;
        }

        auto current_mtime = std::filesystem::last_write_time(file_path);
        auto current_size = std::filesystem::file_size(file_path);

        return (current_mtime == mtime && current_size == size);
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        return false;
    }
}

std::string FileSignature::to_string() const
{
    // 将文件签名转换为字符串，用于生成缓存键
    std::ostringstream oss;
    oss << path << "|" << mtime.time_since_epoch().count() << "|" << size;
    return oss.str();
}

// ============================================================================
// PathCache 实现
// ============================================================================

PathCache::PathCache(const std::string &cache_dir, size_t max_size)
    : cache_dir(cache_dir), max_size(max_size), hit_count(0), miss_count(0)
{
    paths_dir = cache_dir + "/paths";
    index_file_path = cache_dir + "/cache_index.txt";

    init_cache_dir();
    load_index();
}

void PathCache::init_cache_dir()
{
    try
    {
        if (!std::filesystem::exists(cache_dir))
        {
            std::filesystem::create_directories(cache_dir);
        }
        if (!std::filesystem::exists(paths_dir))
        {
            std::filesystem::create_directories(paths_dir);
        }
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        std::cerr << "Error: Could not create cache directory: " << e.what() << std::endl;
    }
}

std::string PathCache::generate_key(const std::string &start,
                                     const std::string &end,
                                     const FileSignature &sig)
{
    // 生成缓存键：组合起点、终点和文件签名，然后计算哈希
    std::string combined = start + "|" + end + "|" + sig.to_string();

    // 使用std::hash生成哈希值
    std::hash<std::string> hasher;
    size_t hash_value = hasher(combined);

    // 转换为16进制字符串
    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(16) << hash_value;
    return oss.str();
}

MultiPath PathCache::get(const std::string &start,
                          const std::string &end,
                          const std::string &csv_file)
{
    MultiPath empty_result;  // 空结果

    // 生成文件签名
    FileSignature sig(csv_file);
    std::string key = generate_key(start, end, sig);

    // 查找缓存条目
    auto it = entries.find(key);
    if (it == entries.end())
    {
        // 缓存未命中
        miss_count++;
        return empty_result;
    }

    // 检查文件签名是否仍然匹配
    if (!it->second.csv_signature.matches(csv_file))
    {
        // 文件已修改，删除过期缓存
        std::filesystem::remove(it->second.cache_file);
        lru_list.remove(key);
        entries.erase(it);
        miss_count++;
        return empty_result;
    }

    // 缓存命中，更新LRU顺序
    touch(key);
    hit_count++;

    // 读取缓存文件
    return read_cache_file(it->second.cache_file);
}

void PathCache::put(const std::string &start,
                    const std::string &end,
                    const std::string &csv_file,
                    const MultiPath &paths)
{
    // 生成文件签名和键
    FileSignature sig(csv_file);
    std::string key = generate_key(start, end, sig);

    // 如果已存在，先删除旧的
    auto it = entries.find(key);
    if (it != entries.end())
    {
        std::filesystem::remove(it->second.cache_file);
        lru_list.remove(key);
        entries.erase(it);
    }

    // 检查是否需要淘汰
    if (entries.size() >= max_size)
    {
        evict_lru();
    }

    // 创建缓存文件
    std::string cache_file = paths_dir + "/" + key + ".cache";
    write_cache_file(cache_file, paths);

    // 创建缓存条目
    CacheEntry entry;
    entry.start = start;
    entry.end = end;
    entry.csv_signature = sig;
    entry.cache_file = cache_file;
    entry.created_at = std::chrono::system_clock::now();

    // 添加到缓存
    entries[key] = entry;
    lru_list.push_front(key);

    // 保存索引
    save_index();
}

void PathCache::clear()
{
    // 删除所有缓存文件
    for (const auto &pair : entries)
    {
        std::filesystem::remove(pair.second.cache_file);
    }

    // 清空数据结构
    entries.clear();
    lru_list.clear();

    // 删除索引文件
    if (std::filesystem::exists(index_file_path))
    {
        std::filesystem::remove(index_file_path);
    }

    hit_count = 0;
    miss_count = 0;
}

void PathCache::evict_lru()
{
    if (lru_list.empty())
    {
        return;
    }

    // 获取最久未使用的键（列表末尾）
    std::string oldest_key = lru_list.back();
    lru_list.pop_back();

    // 删除缓存文件和条目
    auto it = entries.find(oldest_key);
    if (it != entries.end())
    {
        std::filesystem::remove(it->second.cache_file);
        entries.erase(it);
    }
}

void PathCache::touch(const std::string &key)
{
    // 从列表中移除（无论在哪个位置）
    lru_list.remove(key);
    // 添加到最前面
    lru_list.push_front(key);
}

MultiPath PathCache::read_cache_file(const std::string &file_path)
{
    MultiPath paths;
    std::ifstream file(file_path);

    if (!file.is_open())
    {
        return paths;
    }

    std::string line;
    std::vector<std::string> *current_path = nullptr;

    while (std::getline(file, line))
    {
        // 移除可能的回车符
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        if (line.empty())
        {
            continue;  // 跳过空行
        }

        // 检查是否是路径类型标记
        if (line == "# TIME")
        {
            current_path = &paths.time_path;
        }
        else if (line == "# DISTANCE")
        {
            current_path = &paths.distance_path;
        }
        else if (line == "# BALANCED")
        {
            current_path = &paths.balanced_path;
        }
        else if (current_path != nullptr)
        {
            // 添加节点到当前路径
            current_path->push_back(line);
        }
    }

    file.close();
    return paths;
}

void PathCache::write_cache_file(const std::string &file_path, const MultiPath &paths)
{
    std::ofstream file(file_path);

    if (!file.is_open())
    {
        std::cerr << "Error: Could not create cache file " << file_path << std::endl;
        return;
    }

    // 写入时间最短路径
    file << "# TIME\n";
    for (const auto &node : paths.time_path)
    {
        file << node << "\n";
    }

    // 写入距离最短路径
    file << "# DISTANCE\n";
    for (const auto &node : paths.distance_path)
    {
        file << node << "\n";
    }

    // 写入综合推荐路径
    file << "# BALANCED\n";
    for (const auto &node : paths.balanced_path)
    {
        file << node << "\n";
    }

    file.close();
}

void PathCache::load_index()
{
    if (!std::filesystem::exists(index_file_path))
    {
        return;
    }

    std::ifstream file(index_file_path);
    if (!file.is_open())
    {
        return;
    }

    // 简化的文本格式索引文件
    // 格式：
    // max_size: N
    // entry_count: M
    // lru_order: key1,key2,key3,...
    // entry: key|start|end|csv_path|mtime|size|cache_file|created_time
    // entry: ...

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        size_t colon_pos = line.find(':');
        if (colon_pos == std::string::npos)
        {
            continue;
        }

        std::string key_part = line.substr(0, colon_pos);
        std::string value_part = line.substr(colon_pos + 1);

        // 去除前后空格
        value_part.erase(0, value_part.find_first_not_of(" \t\r\n"));
        value_part.erase(value_part.find_last_not_of(" \t\r\n") + 1);

        if (key_part == "max_size")
        {
            // 读取max_size（可选，当前使用构造函数参数）
        }
        else if (key_part == "lru_order")
        {
            // 解析LRU顺序
            std::stringstream ss(value_part);
            std::string key;
            while (std::getline(ss, key, ','))
            {
                if (!key.empty())
                {
                    lru_list.push_back(key);
                }
            }
        }
        else if (key_part == "entry")
        {
            // 解析缓存条目
            // 格式: key|start|end|csv_path|mtime|size|cache_file|created_time
            std::stringstream ss(value_part);
            std::vector<std::string> parts;
            std::string part;
            while (std::getline(ss, part, '|'))
            {
                parts.push_back(part);
            }

            if (parts.size() >= 7)
            {
                std::string key = parts[0];
                CacheEntry entry;
                entry.start = parts[1];
                entry.end = parts[2];
                entry.csv_signature.path = parts[3];

                try
                {
                    // 解析mtime
                    long long mtime_count = std::stoll(parts[4]);
                    entry.csv_signature.mtime = std::filesystem::file_time_type(
                        std::filesystem::file_time_type::duration(mtime_count));

                    // 解析size
                    entry.csv_signature.size = std::stoull(parts[5]);

                    entry.cache_file = parts[6];

                    // 解析created_time（如果有）
                    if (parts.size() >= 8)
                    {
                        long long created_time = std::stoll(parts[7]);
                        entry.created_at = std::chrono::system_clock::time_point(
                            std::chrono::system_clock::duration(created_time));
                    }

                    // 验证缓存文件是否存在
                    if (std::filesystem::exists(entry.cache_file))
                    {
                        entries[key] = entry;
                    }
                }
                catch (const std::exception &e)
                {
                    // 跳过无效条目
                    continue;
                }
            }
        }
    }

    file.close();

    // 清理LRU列表中不存在的条目
    auto it = lru_list.begin();
    while (it != lru_list.end())
    {
        if (entries.find(*it) == entries.end())
        {
            it = lru_list.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void PathCache::save_index()
{
    std::ofstream file(index_file_path);
    if (!file.is_open())
    {
        std::cerr << "Error: Could not save cache index to " << index_file_path << std::endl;
        return;
    }

    // 写入元数据
    file << "# PathCache Index File\n";
    file << "max_size: " << max_size << "\n";
    file << "entry_count: " << entries.size() << "\n";

    // 写入LRU顺序
    file << "lru_order: ";
    bool first = true;
    for (const auto &key : lru_list)
    {
        if (!first)
        {
            file << ",";
        }
        file << key;
        first = false;
    }
    file << "\n";

    // 写入每个缓存条目
    for (const auto &pair : entries)
    {
        const std::string &key = pair.first;
        const CacheEntry &entry = pair.second;

        file << "entry: " << key << "|"
             << entry.start << "|"
             << entry.end << "|"
             << entry.csv_signature.path << "|"
             << entry.csv_signature.mtime.time_since_epoch().count() << "|"
             << entry.csv_signature.size << "|"
             << entry.cache_file << "|"
             << entry.created_at.time_since_epoch().count() << "\n";
    }

    file.close();
}
