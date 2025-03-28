#include "env_loader.h"
#include <fstream>
#include <sstream>
#include <iostream>

std::unordered_map<std::string, std::string> load_env_file(const std::string& path) {
    std::unordered_map<std::string, std::string> env;
    std::ifstream file(path);
    std::string line;

    while (std::getline(file, line)) {
        // Ignore commentaires ou lignes vides
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string key, value;

        if (std::getline(iss, key, '=') && std::getline(iss, value)) {
            env[key] = value;
        }
    }

    return env;
}
