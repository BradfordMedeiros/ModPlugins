#ifndef MODPLUGIN_SQLPARSE
#define MODPLUGIN_SQLPARSE

#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <variant>
#include <stdio.h>
#include "./util.h"

struct SelectToken {};
struct CreateToken {};
struct DropToken {};
struct TableToken {};
struct FromToken {};
struct SpliceToken {};
struct IdentifierToken{
  std::string content;
};
typedef std::variant<SelectToken, CreateToken, DropToken, TableToken, FromToken, SpliceToken, IdentifierToken> LexTokens;

std::string tokenTypeStr(std::vector<LexTokens> tokens, bool includeContent);
std::vector<LexTokens> lex(std::string value);

struct TokenResult {
  bool isDelimiter;
  char delimiter;
  std::string token;
};

std::string tokenizeTypeStr(std::vector<TokenResult> tokens);
std::vector<TokenResult> tokenize(std::string str, std::vector<char> delimiters);
bool createParser(std::vector<LexTokens> lexTokens);

#endif