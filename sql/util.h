#ifndef MODPLUGIN_SQLUTIL
#define MODPLUGIN_SQLUTIL

#include <variant>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <assert.h>

std::string join(std::vector<std::string> values, char delimeter);
void saveFile(std::string filepath, std::string content);
std::string loadFile(std::string filepath);
std::vector<std::string> split(std::string strToSplit, char delimeter);
void appendFile(std::string filepath, std::string content);

#endif