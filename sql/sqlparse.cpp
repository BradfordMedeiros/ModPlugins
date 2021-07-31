#include "./sqlparse.h"

std::string toUpper(std::string s){
  transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return toupper(c); });
  return s;
}

std::string tokenTypeStr(LexTokens token, bool includeContent){
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

  auto createToken = std::get_if<CreateToken>(&token);
  if (createToken != NULL){
    return "CREATE_TOKEN";
  }

  auto dropToken = std::get_if<DropToken>(&token);
  if (dropToken != NULL){
    return "DROP_TOKEN";
  }

  auto tableToken = std::get_if<TableToken>(&token);
  if (tableToken != NULL){
    return "TABLE_TOKEN";
  }

  auto identifierToken = std::get_if<IdentifierToken>(&token);
  if (identifierToken != NULL){
    std::string result =  "IDENTIFIER_TOKEN";
    if (includeContent){
      result = result + "(" + identifierToken -> content + ")";
    }
    return result;
  }

  assert(false);
  return "";
}
std::string tokenTypeStr(std::vector<LexTokens> tokens, bool includeContent){
  std::string content = "";
  for (int i = 0; i < tokens.size(); i++){
    auto token = tokens.at(i);
    content = content + tokenTypeStr(token, includeContent) + (i < (tokens.size() - 1) ? " " : "");
  }
  return content;
}

bool isIdentifier(std::string token){
  return true;
}

std::string tokenTypeStr(TokenResult token){
  if (token.isDelimiter){
    return std::string("DEL(") + token.delimiter + ")";
  }
  return std::string("TOKEN(" + token.token + ")");
}
std::string tokenizeTypeStr(std::vector<TokenResult> tokens){
  std::string content = "";
  for (int i = 0; i < tokens.size(); i++){
    auto token = tokens.at(i);
    content = content + tokenTypeStr(token) + (i < (tokens.size() - 1) ? " " : "");
  }
  return content;
}

std::vector<TokenResult> tokenize(std::string str, std::vector<char> delimiters){
  std::vector<TokenResult> result;

  int lowIndex = 0;
  for (int i = 0; i < str.size(); i++){
    auto currentChar = str.at(i);
    bool isDelimiter = std::count(delimiters.begin(), delimiters.end(), currentChar) > 0;
    if (isDelimiter){
      auto token = str.substr(lowIndex, (i - lowIndex));
      lowIndex = i + 1;
      if (token.size() > 0){
        result.push_back(TokenResult{
          .isDelimiter = false,
          .delimiter = ' ',
          .token = token,
        });
      }
      result.push_back(TokenResult{
        .isDelimiter = true,
        .delimiter = currentChar,
        .token = "",
      });
    }
  }
  if (lowIndex < str.size()){
    auto token = str.substr(lowIndex, str.size() - lowIndex);
    result.push_back(TokenResult{
      .isDelimiter = false,
      .delimiter = ' ',
      .token = token,
    });
  }
  return result;
}

std::vector<LexTokens> lex(std::string value){
  std::vector<LexTokens> lexTokens;
  std::vector<TokenResult> filteredTokens;
  for (auto token : tokenize(value, {' ', ',' })){
    if (token.isDelimiter && token.delimiter == ' '){
      continue;
    }
    filteredTokens.push_back(token);
  }
  for (auto token : filteredTokens){
    if (token.isDelimiter){
      assert(token.delimiter == ',');
      lexTokens.push_back(SpliceToken{});
    }else{
      if (toUpper(token.token) == "SELECT"){
        lexTokens.push_back(SelectToken{});
      }else if (toUpper(token.token) == "FROM"){
        lexTokens.push_back(FromToken{});
      }else if (toUpper(token.token) == "CREATE"){
        lexTokens.push_back(CreateToken{});
      }else if (toUpper(token.token) == "DROP"){
        lexTokens.push_back(DropToken{});
      }else if (toUpper(token.token) == "TABLE"){
        lexTokens.push_back(TableToken{});
      }else if (isIdentifier(token.token)){
        lexTokens.push_back(IdentifierToken{
          .content = token.token,
        });
      }else{
        assert(false);
      }
    }
  }
  return lexTokens;
}

struct TokenState {
  std::vector<std::string> nextStates;
  std::function<void(SqlQuery&, LexTokens)> fn;
};

SqlQuery createParser(std::vector<LexTokens> lexTokens){
  std::map<std::string, TokenState> machine;

  machine["start"] = TokenState{
    .nextStates = { "CREATE_TOKEN", "DROP_TOKEN" },
    .fn = [](SqlQuery& query, LexTokens token) -> void {
    },
  };
  machine["CREATE_TOKEN"] = { 
    .nextStates = { "TABLE_TOKEN" },
    .fn = [](SqlQuery& query, LexTokens token) -> void {
      query.type = SQL_CREATE_TABLE;
    },
  };
  machine["DROP_TOKEN"] = { 
    .nextStates = { "TABLE_TOKEN" }, 
    .fn = [](SqlQuery& query, LexTokens token) -> void {
      query.type = SQL_DELETE_TABLE;
    },
  };
  machine["TABLE_TOKEN"] = { 
    .nextStates = { "IDENTIFIER_TOKEN" }, 
    .fn = [](SqlQuery& query, LexTokens token) -> void {
      auto identifier = std::get_if<IdentifierToken>(&token);
      assert(identifier != NULL);
      query.table = identifier -> content;
    },
  };
  machine["IDENTIFIER_TOKEN" ] = { 
    .nextStates = { "*END*" },
    .fn = [](SqlQuery& query, LexTokens token) -> void {},
  };

  SqlQuery query {
    .validQuery = false,
  };
  std::string currState = "start";
  for (auto lexToken : lexTokens){
    auto nextStates = machine.at(currState).nextStates;
    machine.at(currState).fn(query, lexToken);
    auto tokenAsStr = tokenTypeStr(lexToken, false);
    bool nextStateValid = std::count(nextStates.begin(), nextStates.end(), tokenAsStr) > 0;
    if (!nextStateValid){
      return query;
    }
    currState = tokenAsStr;
  }

  auto finalNextStates = machine.at(currState).nextStates;
  auto completeExpression = std::count(finalNextStates.begin(), finalNextStates.end(), "*END*") > 0;
  query.validQuery = completeExpression;
  return query;
}

SqlQuery compileSqlQuery(std::string queryString){
  return createParser(lex(queryString));
}