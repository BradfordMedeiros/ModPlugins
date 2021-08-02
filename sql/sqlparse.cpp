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
"start CREATE\n"
"start DROP\n"
"start SELECT\n"
"start SHOW\n"
"start DESCRIBE\n"
"start INSERT\n"

"CREATE TABLE\n"
"DROP TABLE\n"
"TABLE IDENTIFIER_TOKEN table\n"
"IDENTIFIER_TOKEN:table *END*\n"

"SHOW TABLES\n"
"TABLES *END*\n"

"SELECT IDENTIFIER_TOKEN select\n"
"IDENTIFIER_TOKEN:select SPLICE\n"
"IDENTIFIER_TOKEN:select FROM\n"
"SPLICE IDENTIFIER_TOKEN select\n"
"FROM IDENTIFIER_TOKEN tableselect\n"
"IDENTIFIER_TOKEN:tableselect LIMIT tableselect\n"
"IDENTIFIER_TOKEN:tableselect WHERE tableselect\n"
"IDENTIFIER_TOKEN:tableselect *END*\n"
"WHERE:tableselect IDENTIFIER_TOKEN whereselect\n"
"IDENTIFIER_TOKEN:whereselect EQUAL whereselect\n"
"EQUAL:whereselect IDENTIFIER_TOKEN whereselect2\n"
"IDENTIFIER_TOKEN:whereselect2 *END*\n"
"IDENTIFIER_TOKEN:whereselect2 LIMIT tableselect\n"
"LIMIT:tableselect IDENTIFIER_TOKEN limit_tableselect\n"
"IDENTIFIER_TOKEN:limit_tableselect *END*\n"

"INSERT INTO\n"
"INTO IDENTIFIER_TOKEN tableinsert\n"
"IDENTIFIER_TOKEN:tableinsert LEFTP tableinsert\n"
"LEFTP:tableinsert IDENTIFIER_TOKEN tableinsertcolname\n"
"IDENTIFIER_TOKEN:tableinsertcolname RIGHTP tableinsertcolname\n"
"RIGHTP:tableinsertcolname VALUES\n"
"VALUES LEFTP tableinsert_v\n"
"LEFTP:tableinsert_v IDENTIFIER_TOKEN tableinsert_v\n"
"IDENTIFIER_TOKEN:tableinsert_v RIGHTP tableinsert_v\n"
"RIGHTP:tableinsert_v *END*\n"

"DESCRIBE IDENTIFIER_TOKEN describe\n"
"IDENTIFIER_TOKEN:describe *END*\n"

"";

std::map<std::string, std::function<void(SqlQuery&, LexTokens* token)>> machineFns {
  {"CREATE", [](SqlQuery& query, LexTokens* token) -> void {
      query.type = SQL_CREATE_TABLE;
      query.queryData = SqlCreate{};
  }},
  {"DROP", [](SqlQuery& query, LexTokens* token) -> void {
      query.type = SQL_DELETE_TABLE;
      query.queryData = SqlDelete{};
  }},
  {"SHOW", [](SqlQuery& query, LexTokens* token) -> void {
      query.type = SQL_SHOW_TABLES;
      query.queryData = SqlShowTables{};
  }},
  {"IDENTIFIER_TOKEN:table", [](SqlQuery& query, LexTokens* token) -> void {
      auto identifier = std::get_if<IdentifierToken>(token);
      assert(identifier != NULL);
      query.table = identifier -> content;
  }},
  {"SELECT", [](SqlQuery& query, LexTokens* token) -> void {
      query.type = SQL_SELECT;
      query.queryData = SqlSelect{
        .limit = -1,
      };
  }},
  {"IDENTIFIER_TOKEN:select", [](SqlQuery& query, LexTokens* token) -> void {
      auto identifierToken = std::get_if<IdentifierToken>(token);
      assert(identifierToken != NULL);
      SqlSelect* selectQuery = std::get_if<SqlSelect>(&query.queryData);
      assert(selectQuery != NULL);
      selectQuery -> columns.push_back(identifierToken -> content);
  }},
  {"IDENTIFIER_TOKEN:tableselect", [](SqlQuery& query, LexTokens* token) -> void {
      auto identifierToken = std::get_if<IdentifierToken>(token);
      assert(identifierToken != NULL);
      query.table = identifierToken -> content;
  }},
  {"IDENTIFIER_TOKEN:limit_tableselect", [](SqlQuery& query, LexTokens* token) -> void {
      auto identifierToken = std::get_if<IdentifierToken>(token);
      assert(identifierToken != NULL);
      SqlSelect* selectQuery = std::get_if<SqlSelect>(&query.queryData);
      assert(selectQuery != NULL);
      selectQuery -> limit = std::atoi(identifierToken -> content.c_str()); // strong typing should occur earlier
  }},
  {"IDENTIFIER_TOKEN:whereselect", [](SqlQuery& query, LexTokens* token) -> void {
      auto identifierToken = std::get_if<IdentifierToken>(token);
      assert(identifierToken != NULL);
      SqlSelect* selectQuery = std::get_if<SqlSelect>(&query.queryData);
      assert(selectQuery != NULL);
      selectQuery -> filter.hasFilter = true;
      selectQuery -> filter.column = identifierToken -> content;
  }},
  {"IDENTIFIER_TOKEN:whereselect2", [](SqlQuery& query, LexTokens* token) -> void {
      auto identifierToken = std::get_if<IdentifierToken>(token);
      assert(identifierToken != NULL);
      SqlSelect* selectQuery = std::get_if<SqlSelect>(&query.queryData);
      assert(selectQuery != NULL);
      selectQuery -> filter.hasFilter = true;
      selectQuery -> filter.value = identifierToken -> content;
  }},
  {"DESCRIBE", [](SqlQuery& query, LexTokens* token) -> void {
      query.type = SQL_DESCRIBE;
      query.queryData = SqlDescribe{};
  }},
  {"IDENTIFIER_TOKEN:describe", [](SqlQuery& query, LexTokens* token) -> void {
      auto identifierToken = std::get_if<IdentifierToken>(token);
      assert(identifierToken != NULL);
      query.table = identifierToken -> content;
  }},
  {"INSERT", [](SqlQuery& query, LexTokens* token) -> void {
      query.type = SQL_INSERT;
      query.queryData = SqlInsert{};
  }},
  {"IDENTIFIER_TOKEN:tableinsert", [](SqlQuery& query, LexTokens* token) -> void {
      auto identifierToken = std::get_if<IdentifierToken>(token);
      assert(identifierToken != NULL);
      query.table = identifierToken -> content;
  }},
  {"IDENTIFIER_TOKEN:tableinsertcolname", [](SqlQuery& query, LexTokens* token) -> void {
      auto insertQuery = std::get_if<SqlInsert>(&query.queryData);
      assert(insertQuery != NULL);
      auto identifierToken = std::get_if<IdentifierToken>(token);
      assert(identifierToken != NULL);
      insertQuery -> columns.push_back(identifierToken -> content);   
  }},
  {"IDENTIFIER_TOKEN:tableinsert_v", [](SqlQuery& query, LexTokens* token) -> void {
      auto insertQuery = std::get_if<SqlInsert>(&query.queryData);
      assert(insertQuery != NULL);
      auto identifierToken = std::get_if<IdentifierToken>(token);
      assert(identifierToken != NULL);
      insertQuery -> values.push_back(identifierToken -> content);   
  }}

};

std::map<std::string, TokenState> createMachine(std::string transitionsStr, std::map<std::string, std::function<void(SqlQuery&, LexTokens* token)>>& fns){
  std::map<std::string, TokenState> machine;
  auto transitions = split(transitionsStr, '\n');
  for (auto transition : transitions){
    auto allTransitions = split(transition, ' ');
    assert(allTransitions.size() == 2 || allTransitions.size() == 3);
    auto machineName = allTransitions.at(0);
    if (machine.find(machineName) == machine.end()){
      machine[machineName] = TokenState{ 
        .nextStates = {},
        .fn = fns.find(machineName) == fns.end() ? [](SqlQuery& query, LexTokens* token) -> void {} : fns.at(machineName),
      };
    }
    machine.at(machineName).nextStates.push_back(NextState{
      .token = allTransitions.at(1),
      .stateSuffix = allTransitions.size() > 2 ? allTransitions.at(2) : "",
    });
  }
  return machine;
}

std::map<std::string, TokenState> machine = createMachine(machineTransitions, machineFns);


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