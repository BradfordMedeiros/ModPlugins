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

  auto leftParenthesisToken = std::get_if<LeftParenthesisToken>(&token);
  if (leftParenthesisToken != NULL){
    return "LEFTP_TOKEN";
  }
  auto rightParenthesisToken = std::get_if<RightParenthesisToken>(&token);
  if (rightParenthesisToken != NULL){
    return "RIGHTP_TOKEN";
  }
  auto insertToken = std::get_if<InsertToken>(&token);
  if (insertToken != NULL){
    return "INSERT_TOKEN";
  }
  auto intoToken = std::get_if<IntoToken>(&token);
  if (intoToken != NULL){
    return "INTO_TOKEN";
  }
  auto valueToken = std::get_if<ValuesToken>(&token);
  if (valueToken != NULL){
    return "VALUE_TOKEN";
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
  for (auto token : tokenize(value, {' ', ',', '(', ')' })){
    if (token.isDelimiter && token.delimiter == ' '){
      continue;
    }
    filteredTokens.push_back(token);
  }
  for (auto token : filteredTokens){
    if (token.isDelimiter){
      if (token.delimiter == ','){
        lexTokens.push_back(SpliceToken{});
      }else if (token.delimiter == '('){
        lexTokens.push_back(LeftParenthesisToken{});
      }else if (token.delimiter == ')'){
        lexTokens.push_back(RightParenthesisToken{});
      }else{
        std::cout << "delimiter: " << token.delimiter << std::endl;
        assert(false);
      }
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
      }else if (toUpper(token.token) == "VALUES"){
        lexTokens.push_back(ValuesToken{});
      }else if (toUpper(token.token) == "INSERT"){
        lexTokens.push_back(InsertToken{});
      }else if (toUpper(token.token) == "INTO"){
        lexTokens.push_back(IntoToken{});
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

struct NextState {
  std::string token;
  std::string stateSuffix;
};
struct TokenState {
  std::vector<NextState> nextStates;
  std::function<void(SqlQuery&, LexTokens*)> fn;
};

std::map<std::string, TokenState> machine = {
  {"start", TokenState{
    .nextStates = {
      NextState { .token = "CREATE_TOKEN", .stateSuffix = "" }, 
      NextState { .token = "DROP_TOKEN", .stateSuffix = "" },
      NextState { .token = "SELECT_TOKEN", .stateSuffix = "" },
    },
    .fn = [](SqlQuery& query, LexTokens* token) -> void {},
  }},
  {"CREATE_TOKEN", TokenState{ 
    .nextStates = { 
      NextState { .token = "TABLE_TOKEN", .stateSuffix = "" },
    },
    .fn = [](SqlQuery& query, LexTokens* token) -> void {
      query.type = SQL_CREATE_TABLE;
    },
  }},
  {"DROP_TOKEN", TokenState{ 
    .nextStates = { 
      NextState { .token = "TABLE_TOKEN", .stateSuffix = "" },
    }, 
    .fn = [](SqlQuery& query, LexTokens* token) -> void {
      query.type = SQL_DELETE_TABLE;
    },
  }},
  {"TABLE_TOKEN", TokenState{ 
    .nextStates = { 
      NextState { .token = "IDENTIFIER_TOKEN", .stateSuffix = "table" },
    }, 
    .fn = [](SqlQuery& query, LexTokens* token) -> void {},
  }},
  {"IDENTIFIER_TOKEN:table", TokenState{ 
    .nextStates = { 
      NextState { .token = "*END*", .stateSuffix = "" },
    },
    .fn = [](SqlQuery& query, LexTokens* token) -> void {
      auto identifier = std::get_if<IdentifierToken>(token);
      assert(identifier != NULL);
      query.table = identifier -> content;
    },
  }},
  {"SELECT_TOKEN", TokenState{ 
    .nextStates = { 
      NextState { .token = "IDENTIFIER_TOKEN", .stateSuffix = "select" },
    },
    .fn = [](SqlQuery& query, LexTokens* token) -> void {
      query.type = SQL_SELECT;
      query.queryData = SqlSelect{};
    },
  }},
  {"IDENTIFIER_TOKEN:select", TokenState{ 
    .nextStates = { 
      NextState { .token = "SPLICE_TOKEN", .stateSuffix = "" },
      NextState { .token = "FROM_TOKEN", .stateSuffix = "" },

    },
    .fn = [](SqlQuery& query, LexTokens* token) -> void {
      auto identifierToken = std::get_if<IdentifierToken>(token);
      assert(identifierToken != NULL);
      SqlSelect* selectQuery = std::get_if<SqlSelect>(&query.queryData);
      assert(selectQuery != NULL);
      selectQuery -> columns.push_back(identifierToken -> content);
    },
  }},
  {"SPLICE_TOKEN", TokenState{ 
    .nextStates = { 
      NextState { .token = "IDENTIFIER_TOKEN", .stateSuffix = "select" },
    },
    .fn = [](SqlQuery& query, LexTokens* token) -> void {},
  }},
  {"FROM_TOKEN", TokenState{ 
    .nextStates = { 
      NextState { .token = "IDENTIFIER_TOKEN", .stateSuffix = "tableselect" },
    },
    .fn = [](SqlQuery& query, LexTokens* token) -> void {},
  }},
  {"IDENTIFIER_TOKEN:tableselect", TokenState{ 
    .nextStates = { 
      NextState { .token = "*END*", .stateSuffix = "" },
    },
    .fn = [](SqlQuery& query, LexTokens* token) -> void {
      auto identifierToken = std::get_if<IdentifierToken>(token);
      assert(identifierToken != NULL);
      query.table = identifierToken -> content;
    },
  }}
};

SqlQuery createParser(std::vector<LexTokens> lexTokens){
  SqlQuery query {
    .validQuery = false,
    .type = SQL_SELECT,
    .table = "",
  };
  std::string currState = "start";
  std::string stateSuffix = "";

  LexTokens* lastToken = NULL;
  for (auto &lexToken : lexTokens){
    if (machine.find(currState) == machine.end()){
      break;
    }
    auto nextStates = machine.at(currState).nextStates;
    machine.at(currState).fn(query, lastToken);
    lastToken = &lexToken;

    auto tokenAsStr = tokenTypeStr(lexToken, false);
    bool nextStateValid = false;
    for (auto state : nextStates){
      if (state.token == tokenAsStr){
        nextStateValid = true;
        stateSuffix = state.stateSuffix;
        break;
      }
    }
    if (!nextStateValid){
      return query;
    }
    currState = tokenAsStr + (stateSuffix.size() == 0 ? "" : (":" + stateSuffix));
  }

  if (machine.find(currState) != machine.end()){
    machine.at(currState).fn(query, lastToken);
    auto finalNextStates = machine.at(currState).nextStates;
    auto completeExpression = false;
    for (auto state : finalNextStates){
      if (state.token == "*END*"){
        completeExpression = true;
        break;
      }
    }
    query.validQuery = completeExpression;
  }
  return query;
}

SqlQuery compileSqlQuery(std::string queryString){
  return createParser(lex(queryString));
}