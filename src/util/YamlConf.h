#ifndef YAMLCONF_H
#define YAMLCONF_H

#include "yaml/include/yaml.h"

class YamlConf
{
  public:
    YamlConf(const char *fileName)
    {
        config = YAML::LoadFile(fileName);
    }
    YamlConf(const std::string fileName)
    {
        config = YAML::LoadFile(fileName);
    }
    ~YamlConf()
    {
    }
    YAML::Node config;

    int isSet(const char *key);
    int isSet(std::string key);
    int getInt(const char *key);
    std::string getString(const char *key);
};

#endif
