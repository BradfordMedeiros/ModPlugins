#include "./sqlparse_test.h"

void assertLex(std::string sqlQuery, std::string expectedLex){
  auto lexString = tokenTypeStr(lex(sqlQuery));
  if (lexString != expectedLex){
    throw std::logic_error("Incorrect lex string\ngot: " + lexString + " \nwanted: " + expectedLex);
  }  
}
void lexTestSelect1(){
  assertLex(
    "select name from users",
    "SELECT_TOKEN IDENTIFIER_TOKEN(name) FROM_TOKEN IDENTIFIER_TOKEN(users)"
  );
}
void lexTestSelect2(){
  assertLex(
    "    select  name  from users    ",
    "SELECT_TOKEN IDENTIFIER_TOKEN(name) FROM_TOKEN IDENTIFIER_TOKEN(users)"
  );
}
void lexTestSelect3(){
  assertLex(
    "select style from fashions",
    "SELECT_TOKEN IDENTIFIER_TOKEN(style) FROM_TOKEN IDENTIFIER_TOKEN(fashions)"
  );
}

void lexTestSelectSplice(){
  assertLex(
    "select name,age from users",
    "SELECT_TOKEN IDENTIFIER_TOKEN(name) SPLICE_TOKEN IDENTIFIER_TOKEN(age) FROM_TOKEN IDENTIFIER_TOKEN(users)"
  );
}

void lexTestSelectSpliceWeirdSpacing(){
  assertLex(
    "select  name ,  age from users",
    "SELECT_TOKEN IDENTIFIER_TOKEN(name) SPLICE_TOKEN IDENTIFIER_TOKEN(age) FROM_TOKEN IDENTIFIER_TOKEN(users)"
  );
}



