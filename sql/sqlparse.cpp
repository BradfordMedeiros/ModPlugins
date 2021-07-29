#include "./sqlparse.h"

std::string toUpper(std::string s){
  transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return toupper(c); });
  return s;
}

std::string tokenTypeStr(LexTokens token){
  auto selectToken = std::get_if<SelectToken>(&token);
  if (selectToken != NULL){
    return "SELECT_TOKEN"; 
  }

  auto fromToken = std::get_if<FromToken>(&token);
  if (fromToken != NULL){
    return "FROM_TOKEN";
  }

  auto spliceToken = std::get_if<SpliceToken>(&token);
  if (spliceToken != NULL){
    return "SPLICE_TOKEN";
  }

  auto identifierToken = std::get_if<IdentifierToken>(&token);
  if (identifierToken != NULL){
    return std::string("IDENTIFIER_TOKEN(") + identifierToken -> content + ")";
  }
  assert(false);
  return "";
}
std::string tokenTypeStr(std::vector<LexTokens> tokens){
  std::string content = "";
  for (int i = 0; i < tokens.size(); i++){
    auto token = tokens.at(i);
    content = content + tokenTypeStr(token) + (i < (tokens.size() - 1) ? " " : "");
  }
  return content;
}

bool isIdentifier(std::string token){
  return true;
}

std::vector<LexTokens> lex(std::string value){
  std::vector<std::string> tokens = filterWhitespace(split(value, ' '));
  std::vector<LexTokens> lexTokens;
  for (auto token : tokens){
    if (toUpper(token) == "SELECT"){
      lexTokens.push_back(SelectToken{});
    }else if (toUpper(token) == "FROM"){
      lexTokens.push_back(FromToken{});
    }else if (token == ","){
      lexTokens.push_back(SpliceToken{});
    }else if (isIdentifier(token)){
      lexTokens.push_back(IdentifierToken{
        .content = token,
      });
    }
  }
  return lexTokens;
}