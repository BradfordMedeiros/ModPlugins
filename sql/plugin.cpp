#include <iostream>
#include <libguile.h>
#include "./sql.h"
/*

std::string mainCommand = scm_to_locale_string(scm_list_ref(sqlQuery, scm_from_int64(0)));
  std::string table = scm_to_locale_string(scm_list_ref(sqlQuery, scm_from_int64(1)));
  SqlQuery query {
    .type = SQL_SELECT,
    .table = table,
  };
  if (mainCommand == "select"){
    std::cout << "setting query to select" << std::endl;
    query.type = SQL_SELECT;
    query.queryData = SqlSelect{
      .columns = listToVecString(scm_list_ref(sqlQuery, scm_from_int64(2))),
      .filter = SqlFilter {
        .hasFilter = false,
        .column = "",
        .value = "",
        .invert = false,
      }
    };
  }else if (mainCommand == "insert"){
    query.type = SQL_INSERT;
    query.queryData = SqlInsert {
      .columns = listToVecString(scm_list_ref(sqlQuery, scm_from_int64(2))),
      .values = listToVecString(scm_list_ref(sqlQuery, scm_from_int64(3))),
    };
  }else if (mainCommand == "update"){
    query.type = SQL_UPDATE;
    std::cout << "WARNING: GENERIC UPDATE, NOT USING ACTUAL VALUES" << std::endl;
    query.queryData = SqlUpdate {
      .columns = { "name" },
      .values = { "no-one" },
      .filter = SqlFilter {
        .hasFilter = true,
        .column = "name",
        .value = "unknown",
        .invert = false,
      }
    };
  }else if (mainCommand == "delete"){
    query.type = SQL_DELETE;
    std::cout << "WARNING: GENERIC DELETE, NOT USING ACTUAL VALUES" << std::endl;
    query.queryData = SqlDelete {
      .filter = SqlFilter {
        .hasFilter = true,
        .column = "description",
        .value = "hello",
        .invert = false,
      }
    };
  }else if (mainCommand == "create-table"){
    std::cout << "setting query to create" << std::endl;
    query.type = SQL_CREATE_TABLE;
    query.queryData = SqlCreate{
      .columns = listToVecString(scm_list_ref(sqlQuery, scm_from_int64(2))),
    };
  }else if (mainCommand == "delete-table"){
    query.type = SQL_DELETE_TABLE;
  }
  std::cout << "INFO: executing sql query" << std::endl;
  auto sqlResponse = executeSqlQuery(query);
  std::cout << "INFO: finished executing query" << std::endl;
  return listToSCM(sqlResponse);*/


/*struct sqlQueryHolder {
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
}*/


SCM scmHelloWorld(){
  std::cout << "hello world" << std::endl;
  return SCM_UNSPECIFIED;
}

void registerGuileFns() asm ("registerGuileFns");
void registerGuileFns() { 
 //scm_c_define_gsubr("sql", 0, 0, 0, (void*)test);
 // scm_c_define_gsubr("sql", 1, 0, 0, (void*)scmSql);
 // scm_c_define_gsubr("sql-compile", 1, 0, 0, (void*)scmSqlCompile);
}

#ifdef BINARY_MODE

int main(){
  std::cout << "sql - hello world" << std::endl;
}

#endif