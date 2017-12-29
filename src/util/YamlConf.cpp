#include "YamlConf.h"

int YamlConf::isSet( const char *key )
{
    if( config[ key ] ) {
        return 1;
    }
    return 0;
}

int YamlConf::isSet( std::string key )
{
    if( config[ key ] ) {
        return 1;
    }
    return 0;
}

int YamlConf::getInt( const char *key )
{
    if( config[ key ] ) {
        return config[ key ].as<int>();
    }
    return -1;
}

std::string YamlConf::getString( const char *key )
{
    if( config[ key ] ) {
        return config[ key ].as<std::string>();
    }
    return "";
}
