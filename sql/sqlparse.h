#ifndef MODPLUGIN_SQLPARSE
#define MODPLUGIN_SQLPARSE

#include <vector>
#include <string>
#include <algorithm>
#include <variant>
#include <stdio.h>
#include "./util.h"

enum SQL_TOKENTYPES { SELECT_TOKEN, FROM_TOKEN, IDENTIFIER_TOKEN };

struct SelectToken {};
struct FromToken {};
struct SpliceToken {};
struct IdentifierToken{
  std::string content;
};
typedef std::variant<SelectToken, FromToken, SpliceToken, IdentifierToken> LexTokens;

std::string tokenTypeStr(std::vector<LexTokens> tokens);
std::vector<LexTokens> lex(std::string value);


struct TokenResult {
  bool isDelimiter;
  char delimiter;
  std::string token;
};

std::string tokenizeTypeStr(std::vector<TokenResult> tokens);
std::vector<TokenResult> tokenize(std::string str, std::vector<char> delimiters);

#endif