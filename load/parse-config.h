#ifndef PARSE_CONFIG_H
#define PARSE_CONFIG_H

#include <stdio.h>

#define PCFG_MODE_READONLY  "ro"

typedef FILE* pcfg_ConfigFile;

pcfg_ConfigFile pcfg_openConfigFile(const char *filepath);
void pcfg_getValue(void *var, pcfg_ConfigFile cfgFile, const char* cfgName);
void pcfg_closeConfigFile(pcfg_ConfigFile cfgFile);

#endif
