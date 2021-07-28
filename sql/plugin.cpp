#include <iostream>
#include <libguile.h>
#include "./sql.h"

SCM nestedVecToSCM(std::vector<std::vector<std::string>>& list){
  SCM scmList = scm_make_list(scm_from_unsigned_integer(list.size()), scm_from_unsigned_integer(0));
  for (int i = 0; i < list.size(); i++){
    auto sublist = list.at(i);
    SCM scmSubList = scm_make_list(scm_from_unsigned_integer(sublist.size()), scm_from_unsigned_integer(0));
    for (int j = 0; j < sublist.size(); j++){
      scm_list_set_x (scmSubList, scm_from_unsigned_integer(j), scm_from_locale_string(sublist.at(j).c_str()));
    }
    scm_list_set_x(scmList, scm_from_unsigned_integer(i), scmSubList);
  }
  return scmList;
}

struct sqlQueryHolder {
  SqlQuery* query;
};

SCM sqlObjectType;  

SqlQuery* queryFromForeign(SCM sqlQuery){
  sqlQueryHolder* query;
  scm_assert_foreign_object_type(sqlObjectType, sqlQuery);
  query = (sqlQueryHolder*)scm_foreign_object_ref(sqlQuery, 0); 
  return query -> query;
}

SCM scmSql(SCM sqlQuery){
  auto query = queryFromForeign(sqlQuery);
  auto sqlResult = executeSqlQuery(*query);
  return nestedVecToSCM(sqlResult);
} 

SCM scmSqlCompile(SCM sqlQueryString){
  auto queryObj = (sqlQueryHolder*)scm_gc_malloc(sizeof(sqlQueryHolder), "sqlquery");
  SqlQuery* query = new SqlQuery;
  *query = compileSqlQuery(scm_to_locale_string(sqlQueryString));
  queryObj -> query = query;
  return scm_make_foreign_object_1(sqlObjectType, queryObj);
}

void finalizeSqlObjectType(SCM sqlQuery){
  auto query = queryFromForeign(sqlQuery);
  delete query;
}

void registerGuileFns() asm ("registerGuileFns");
void registerGuileFns() { 
 scm_c_define_gsubr("sql", 1, 0, 0, (void*)scmSql);
 scm_c_define_gsubr("sql-compile", 1, 0, 0, (void*)scmSqlCompile);
}

void registerGuileTypes() asm("registerGuileTypes");
void registerGuileTypes(){
  sqlObjectType = scm_make_foreign_object_type(scm_from_utf8_symbol("sqlquery"), scm_list_1(scm_from_utf8_symbol("data")), finalizeSqlObjectType);
}

#ifdef BINARY_MODE

int main(){

  std::cout << "sql - hello world" << std::endl;
}

#endif