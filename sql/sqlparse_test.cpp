#include "./sqlparse_test.h"

void assertTokenization(std::string str, std::vector<char> delimitors, std::string expectedTokenStr){
  std::string tokString = tokenizeTypeStr(tokenize(str, delimitors));
  if (expectedTokenStr != tokString){
    throw std::logic_error("Incorrect tok string\ngot: " + tokString + " \nwanted: " + expectedTokenStr);
  }  
}
void tokenize1(){
  assertTokenization("hello-world-yo", {'-'}, "TOKEN(hello) DEL(-) TOKEN(world) DEL(-) TOKEN(yo)");
}
void tokenize2(){
  assertTokenization("hello+world-yo", {'+'}, "TOKEN(hello) DEL(+) TOKEN(world-yo)");
}
void tokenize3(){
  assertTokenization("hello+world-yo", {'+','-'}, "TOKEN(hello) DEL(+) TOKEN(world) DEL(-) TOKEN(yo)");
}
void tokenize4(){
  assertTokenization("", {'-'}, "");
}
void tokenize5(){
  assertTokenization("-", {'-'}, "DEL(-)");
}
void tokenize6(){
  assertTokenization("--", {'-'}, "DEL(-) DEL(-)");
}
void tokenize7(){
  assertTokenization("+-+", {'-'}, "TOKEN(+) DEL(-) TOKEN(+)");
}

void assertLex(std::string sqlQuery, std::string expectedLex){
  auto lexString = tokenTypeStr(lex(sqlQuery), true);
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
    "select  name ,  age from users create table drop table ",
    "SELECT_TOKEN IDENTIFIER_TOKEN(name) SPLICE_TOKEN IDENTIFIER_TOKEN(age) FROM_TOKEN IDENTIFIER_TOKEN(users) CREATE_TOKEN TABLE_TOKEN DROP_TOKEN TABLE_TOKEN"
  );
}

void assertComplete(std::string expression, bool expected){
  bool complete = createParser(lex(expression));
  if (complete != expected){
    throw std::logic_error(std::string("Incorrect completeness\ngot: ") + std::to_string(complete) + " \nwanted: " + std::to_string(expected));
  }
}

void testParserComplete(){
  assertComplete("create table sometable", true);
  assertComplete("create table anothertable", true);
  assertComplete("drop table anothertable", true);
}

void testParserIncomplete(){
  assertComplete("create", false);
  assertComplete("table anothertable", false);
  assertComplete("blob table anothertable", false);
}




