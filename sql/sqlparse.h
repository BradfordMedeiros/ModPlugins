#ifndef MODPLUGIN_SQLPARSE
#define MODPLUGIN_SQLPARSE

#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <functional>
#include <variant>
#include <stdio.h>
#include "./util.h"

struct SelectToken {};
struct CreateToken {};
struct DropToken {};
struct TableToken {};
struct FromToken {};
struct SpliceToken {};
struct InsertToken {};
struct IntoToken {};
struct ValuesToken {};
struct LeftParenthesisToken {};
struct RightParenthesisToken {};

struct IdentifierToken{
  std::string content;
};
typedef std::variant<
  SelectToken, CreateToken, DropToken, TableToken, FromToken, 
  IdentifierToken, 
  SpliceToken, LeftParenthesisToken, RightParenthesisToken,
  InsertToken, IntoToken, ValuesToken
> LexTokens;

std::string tokenTypeStr(std::vector<LexTokens> tokens, bool includeContent);
std::vector<LexTokens> lex(std::string value);

struct TokenResult {
  bool isDelimiter;
  char delimiter;
  std::string token;
};

std::string tokenizeTypeStr(std::vector<TokenResult> tokens);
std::vector<TokenResult> tokenize(std::string str, std::vector<char> delimiters);


enum SQL_QUERY_TYPE { SQL_SELECT, SQL_INSERT, SQL_UPDATE, SQL_DELETE, SQL_CREATE_TABLE, SQL_DELETE_TABLE };

struct SqlFilter {
  bool hasFilter;
  std::string column;
  std::string value;
  bool invert;
};

struct SqlSelect {
  std::vector<std::string> columns;
  SqlFilter filter;
};
struct SqlInsert {
  std::vector<std::string> columns;
  std::vector<std::string> values;
};
struct SqlCreate {
  std::vector<std::string> columns;
};
struct SqlUpdate {
  std::vector<std::string> columns;
  std::vector<std::string> values;
  SqlFilter filter;
};
struct SqlDelete {
  SqlFilter filter;
};

struct SqlQuery {
  bool validQuery;
  SQL_QUERY_TYPE type;
  std::string table;
  std::variant<SqlSelect, SqlInsert, SqlCreate, SqlUpdate, SqlDelete> queryData;
};
SqlQuery createParser(std::vector<LexTokens> lexTokens);

SqlQuery compileSqlQuery(std::string queryString);

#endif