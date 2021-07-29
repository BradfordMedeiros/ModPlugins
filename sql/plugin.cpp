#include <iostream>
#include <libguile.h>
#include <iostream>
#include "./sql.h"
#include "./sqlparse.h"

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

void sampleTest1(){
  throw std::logic_error("fail");
}
void sampleTest2(){
}

typedef void (*func_t)();
struct TestCase {
  const char* name;
  func_t test;
};

int main(){
  std::vector<TestCase> tests = { 
    TestCase {
      .name = "test1",
      .test = sampleTest1,
    },
    TestCase {
      .name = "test2",
      .test = sampleTest2,
    },
  };

  int totalTests = tests.size();
  int numFailures = 0;
  for (int i = 0; i < tests.size(); i++){
    auto test = tests.at(i);
    try {
      test.test();
      std::cout << i << " : " << test.name << " : pass" << std::endl;
    }catch(...){
      std::cout << i << " : " << test.name << " : fail" << std::endl;
      numFailures++;
    }
  }

  auto tokens = lex("select name from users    ");
  
  std::cout << "tokens: (" << tokens.size() << ") :" ;
  for (auto token : tokens){
    std::cout << tokenTypeStr(token) << " ";
  }
  std::cout << std::endl;

  std::cout << "Tests passing: " << (totalTests - numFailures) << " / " << totalTests << std::endl;
}

#endif