#include "./sqlparse.h"

std::string toUpper(std::string s){
  transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return toupper(c); });
  return s;
}

std::string tokenTypeStr(LexTokens token, bool includeContent){
  auto symbolToken = std::get_if<SymbolToken>(&token);
  if (symbolToken != NULL){
    return symbolToken -> name; 
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


std::vector<const char*> validSymbols = {
  "SELECT", "FROM", "CREATE", "DROP", "TABLE", "SHOW", "TABLES", "VALUES", "INSERT", "INTO", "DESCRIBE", "GROUP", "BY", "LIMIT", "WHERE"
}; 

std::vector<LexTokens> lex(std::string value){
  std::vector<LexTokens> lexTokens;
  std::vector<TokenResult> filteredTokens;
  for (auto token : tokenize(value, {' ', ',', '(', ')', '=', '"' })){
    if (token.isDelimiter && token.delimiter == ' '){
      continue;
    }
    filteredTokens.push_back(token);
  }
  for (auto token : filteredTokens){
    if (token.isDelimiter){
      if (token.delimiter == ','){
        lexTokens.push_back(SymbolToken { .name = "SPLICE" });
      }else if (token.delimiter == '('){
        lexTokens.push_back(SymbolToken { .name = "LEFTP" });
      }else if (token.delimiter == ')'){
        lexTokens.push_back(SymbolToken { .name = "RIGHTP" });
      }else if (token.delimiter == '='){
        lexTokens.push_back(SymbolToken { .name = "EQUAL" });
      }else if (token.delimiter == '\"'){
        lexTokens.push_back(SymbolToken { .name = "QUOTE" });
      }else{
        std::cout << "delimiter: " << token.delimiter << std::endl;
        assert(false);
      }
    }else{
      bool isSymbol = false;
      for (auto validSymbol : validSymbols){
        if (toUpper(token.token) == validSymbol){
          lexTokens.push_back(SymbolToken { .name = validSymbol });
          isSymbol = true;
          break;
        }
      }
      if (!isSymbol){
        if (isIdentifier(token.token)){
          lexTokens.push_back(IdentifierToken{
            .content = token.token,
          });
        }else{
          assert(false);
        }
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

auto machineTransitions = ""
"start describe\n"
"describe tables describe\n"
"tables:describe *END*\n"

"start show\n"
"show tables show\n"
"tables:show *END*\n"
"";

std::map<std::string, TokenState> createMachine(std::string transitionsStr){
  std::map<std::string, TokenState> machine;
  auto transitions = split(transitionsStr, '\n');
  for (auto transition : transitions){
    auto allTransitions = split(transition, ' ');
    assert(allTransitions.size() == 2 || allTransitions.size() == 3);
    auto machineName = allTransitions.at(0);
    if (machine.find(machineName) == machine.end()){
      machine[machineName] = TokenState{ 
        .nextStates = {},
        .fn = [](SqlQuery& query, LexTokens* token) -> void {},
      };
    }
    machine.at(machineName).nextStates.push_back(NextState{
      .token = allTransitions.at(1),
      .stateSuffix = allTransitions.size() > 2 ? allTransitions.at(2) : "",
    });
  }
  return machine;
}

std::map<std::string, std::function<void(SqlQuery&, LexTokens* token)>> machineFns {
  { {"start"}, [](SqlQuery&, LexTokens* token) -> void {
    std::cout << "machine start!" << std::endl;
  }}
};

std::map<std::string, TokenState> machine = {
  {"start", TokenState{
    .nextStates = {
      NextState { .token = "CREATE", .stateSuffix = "" }, 
      NextState { .token = "DROP", .stateSuffix = "" },
      NextState { .token = "SELECT", .stateSuffix = "" },
      NextState { .token = "SHOW", .stateSuffix = "" },\
      NextState { .token = "DESCRIBE", .stateSuffix = "" },\
    },
    .fn = [](SqlQuery& query, LexTokens* token) -> void {},
  }},
  {"CREATE", TokenState{ 
    .nextStates = { 
      NextState { .token = "TABLE", .stateSuffix = "" },
    },
    .fn = [](SqlQuery& query, LexTokens* token) -> void {
      query.type = SQL_CREATE_TABLE;
      query.queryData = SqlCreate{};
    },
  }},
  {"DROP", TokenState{ 
    .nextStates = { 
      NextState { .token = "TABLE", .stateSuffix = "" },
    }, 
    .fn = [](SqlQuery& query, LexTokens* token) -> void {
      query.type = SQL_DELETE_TABLE;
      query.queryData = SqlDelete{};
    },
  }},
  {"TABLE", TokenState{ 
    .nextStates = { 
      NextState { .token = "IDENTIFIER_TOKEN", .stateSuffix = "table" },
    }, 
    .fn = [](SqlQuery& query, LexTokens* token) -> void {},
  }},
  {"SHOW", TokenState{
    .nextStates = { 
      NextState { .token = "TABLES", .stateSuffix = "" },
    }, 
    .fn = [](SqlQuery& query, LexTokens* token) -> void {
      query.type = SQL_SHOW_TABLES;
      query.queryData = SqlShowTables{};
    },
  }},
  {"TABLES", TokenState{ 
    .nextStates = { 
      NextState { .token = "*END*", .stateSuffix = "" },
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
  {"SELECT", TokenState{ 
    .nextStates = { 
      NextState { .token = "IDENTIFIER_TOKEN", .stateSuffix = "select" },
    },
    .fn = [](SqlQuery& query, LexTokens* token) -> void {
      query.type = SQL_SELECT;
      query.queryData = SqlSelect{
        .limit = -1,
      };
    },
  }},
  {"IDENTIFIER_TOKEN:select", TokenState{ 
    .nextStates = { 
      NextState { .token = "SPLICE", .stateSuffix = "" },
      NextState { .token = "FROM", .stateSuffix = "" },

    },
    .fn = [](SqlQuery& query, LexTokens* token) -> void {
      auto identifierToken = std::get_if<IdentifierToken>(token);
      assert(identifierToken != NULL);
      SqlSelect* selectQuery = std::get_if<SqlSelect>(&query.queryData);
      assert(selectQuery != NULL);
      selectQuery -> columns.push_back(identifierToken -> content);
    },
  }},
  {"SPLICE", TokenState{ 
    .nextStates = { 
      NextState { .token = "IDENTIFIER_TOKEN", .stateSuffix = "select" },
    },
    .fn = [](SqlQuery& query, LexTokens* token) -> void {},
  }},
  {"FROM", TokenState{ 
    .nextStates = { 
      NextState { .token = "IDENTIFIER_TOKEN", .stateSuffix = "tableselect" },
    },
    .fn = [](SqlQuery& query, LexTokens* token) -> void {},
  }},
  {"IDENTIFIER_TOKEN:tableselect", TokenState{ 
    .nextStates = { 
      NextState { .token = "LIMIT", .stateSuffix = "tableselect" },
      NextState { .token = "*END*", .stateSuffix = "" },
    },
    .fn = [](SqlQuery& query, LexTokens* token) -> void {
      auto identifierToken = std::get_if<IdentifierToken>(token);
      assert(identifierToken != NULL);
      query.table = identifierToken -> content;
    },
  }},
  {"LIMIT:tableselect", TokenState{ 
    .nextStates = { 
      NextState { .token = "IDENTIFIER_TOKEN", .stateSuffix = "limit_tableselect" },
      NextState { .token = "*END*", .stateSuffix = "" },
    },
    .fn = [](SqlQuery& query, LexTokens* token) -> void {},
  }},
  {"IDENTIFIER_TOKEN:limit_tableselect", TokenState{ 
    .nextStates = { 
      NextState { .token = "*END*", .stateSuffix = "" },
    },
    .fn = [](SqlQuery& query, LexTokens* token) -> void {
      auto identifierToken = std::get_if<IdentifierToken>(token);
      assert(identifierToken != NULL);
      SqlSelect* selectQuery = std::get_if<SqlSelect>(&query.queryData);
      assert(selectQuery != NULL);
      selectQuery -> limit = std::atoi(identifierToken -> content.c_str()); // strong typing should occur earlier
    },  
  }},
  {"DESCRIBE", TokenState{ 
    .nextStates = { 
      NextState { .token = "IDENTIFIER_TOKEN", .stateSuffix = "" },
    },
    .fn = [](SqlQuery& query, LexTokens* token) -> void {
      query.type = SQL_DESCRIBE;
      query.queryData = SqlDescribe{};
    },
  }},
  {"IDENTIFIER_TOKEN", TokenState{ 
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