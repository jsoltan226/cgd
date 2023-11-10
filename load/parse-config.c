#include "parse-config.h"
#include <stdio.h>

pcfg_ConfigFile pcfg_openConfigFile(const char *filepath)
{
    return (pcfg_ConfigFile)fopen(filepath, PCFG_MODE_READONLY);
}

void pcfg_getValue(void *var, pcfg_ConfigFile cfgFile, const char *cfgName)
{
    var = NULL;
}

void pcfg_closeConfigFile(pcfg_ConfigFile cfgFile)
{
    fclose((FILE*)cfgFile);
}
