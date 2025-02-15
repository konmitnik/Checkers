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

    // ������������ json ������� config
    void reload()
    {
        // ��������� ���� settings.json � ��������� ��� ���������� � config, ����� ���� ��������� ����
        std::ifstream fin(project_path + "settings.json");
        fin >> config;
        fin.close();
    }

    // ���������� ��������� ������ ��������� ����� ������ ��������� ������ �� ������� config
    auto operator()(const string &setting_dir, const string &setting_name) const
    {
        return config[setting_dir][setting_name];
    }

  private:
    json config;
};
