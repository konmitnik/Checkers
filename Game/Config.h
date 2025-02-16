#pragma once
#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "../Models/Project_path.h"

class Config
{
public:
    Config()
    {
        reload();
    }

    // перезагрузка json объекта config
    void reload()
    {
        // открывает файл settings.json и выгружает его содержимое в config, после чего закрывает файл
        std::ifstream fin(project_path + "settings.json");
        fin >> config;
        fin.close();
    }

    // перегрузка оператора скобок позволяет более удобно извлекать данные из объекта config
    auto operator()(const string& setting_dir, const string& setting_name) const
    {
        return config[setting_dir][setting_name];
    }

private:
    json config;
};
