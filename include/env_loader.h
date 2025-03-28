#ifndef ENV_LOADER_H
#define ENV_LOADER_H

#include <string>
#include <unordered_map>

std::unordered_map<std::string, std::string> load_env_file(const std::string& path);

#endif  // ENV_LOADER_H
