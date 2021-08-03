#ifndef MODPLUGIN_SQL
#define MODPLUGIN_SQL

#include <string.h>
#include "./sqlparse.h"
#include "./util.h"

std::vector<std::vector<std::string>> executeSqlQuery(SqlQuery& query);

#endif