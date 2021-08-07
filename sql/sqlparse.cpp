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
  auto operatorToken = std::get_if<OperatorToken>(&token);
  if (operatorToken != NULL){
    std::string result = "OPERATOR";
    if (includeContent){
      std::string op = "";
      if (operatorToken -> type == GREATER_THAN){
        op = ">";
      }else if (operatorToken -> type == LESS_THAN){
        op = "<";
      }else if (operatorToken -> type == EQUAL){
        op = "=";
      }else if (operatorToken -> type == NOT_EQUAL){
        op = "!=";
      }else if (operatorToken -> type == GREATER_THAN_OR_EQUAL){
        op = ">=";
      }else if (operatorToken -> type == LESS_THAN_OR_EQUAL){
        op = "<=";
      }
      result = result + "(" + op + ")";
    }
    return result; 
  }

  auto identifierToken = std::get_if<IdentifierToken>(&token);
  if (identifierToken != NULL){
    std::string result =  "IDENTIFIER_TOKEN";
    if (includeContent){
      result = result + "(" + identifierToken -> content + ")";
    }
    return result;
  }

  auto invalidToken = std::get_if<InvalidToken>(&token);
  if (invalidToken != NULL){
    return "INVALID_TOKEN";
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
  "SELECT", "FROM", "CREATE", "DROP", "TABLE", "SHOW", "TABLES", "VALUES", 
  "INSERT", "INTO", "DESCRIBE", "GROUP", "BY", "LIMIT", "WHERE", "UPDATE", "SET",
  "ORDER", "ASC", "DESC",
}; 

std::vector<LexTokens> lex(std::string value){
  std::vector<LexTokens> lexTokens;
  std::vector<TokenResult> filteredTokens;
  for (auto token : tokenize(value, {' ', ',', '(', ')', '=', '"', '<', '>', '!', '\n', '\r' })){
    if (token.isDelimiter && token.delimiter == ' '){
      continue;
    }
    filteredTokens.push_back(token);
  }

  for (int i = 0; i < filteredTokens.size(); i++){
    auto token = filteredTokens.at(i);
    if (token.isDelimiter){
      if (token.delimiter == ','){
        lexTokens.push_back(SymbolToken { .name = "SPLICE" });
      }else if (token.delimiter == '('){
        lexTokens.push_back(SymbolToken { .name = "LEFTP" });
      }else if (token.delimiter == ')'){
        lexTokens.push_back(SymbolToken { .name = "RIGHTP" });
      }else if (token.delimiter == '='){
        lexTokens.push_back(SymbolToken { . name = "EQUAL" });
      }else if (token.delimiter == '!'){
        auto hasNextToken = (i + 1) < filteredTokens.size();
        auto nextTokenEqual = hasNextToken && (filteredTokens.at(i + 1).isDelimiter && filteredTokens.at(i + 1).delimiter == '=');
        if (nextTokenEqual){
          lexTokens.push_back(OperatorToken { .type = NOT_EQUAL });
          i = i + 1;
        }else{
          lexTokens.push_back(InvalidToken{});
        }
      }else if (token.delimiter == '>'){
        auto hasNextToken = (i + 1) < filteredTokens.size();
        auto nextTokenEqual = hasNextToken && (filteredTokens.at(i + 1).isDelimiter && filteredTokens.at(i + 1).delimiter == '=');
        if (nextTokenEqual){
          lexTokens.push_back(OperatorToken { .type = GREATER_THAN_OR_EQUAL });
          i = i + 1; // skip the next token
        }else{
          lexTokens.push_back(OperatorToken { .type = GREATER_THAN });
        }
      }else if (token.delimiter == '<'){
        auto hasNextToken = (i + 1) < filteredTokens.size();
        auto nextTokenEqual = hasNextToken && (filteredTokens.at(i + 1).isDelimiter && filteredTokens.at(i + 1).delimiter == '=');
        if (nextTokenEqual){
          lexTokens.push_back(OperatorToken { .type = LESS_THAN_OR_EQUAL });
          i = i + 1; // skip the next token
        }else{
          lexTokens.push_back(OperatorToken { .type = LESS_THAN });
        }
      }else if (token.delimiter == '\"'){
        lexTokens.push_back(SymbolToken { .name = "QUOTE" });
      }else if (token.delimiter == '\n' || token.delimiter == '\r'){
        // do nothing, this just gets ignored
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
"start UPDATE\n"

"CREATE TABLE\n"
"TABLE IDENTIFIER_TOKEN table\n"
"IDENTIFIER_TOKEN:table *END*\n"
"IDENTIFIER_TOKEN:table LEFTP table\n"
"LEFTP:table IDENTIFIER_TOKEN create_tablecol\n"
"IDENTIFIER_TOKEN:create_tablecol SPLICE create_tablecol\n"
"SPLICE:create_tablecol IDENTIFIER_TOKEN create_tablecol\n"
"IDENTIFIER_TOKEN:create_tablecol RIGHTP create_tablecol\n"
"RIGHTP:create_tablecol *END*\n"

"DROP TABLE droptable\n"
"TABLE:droptable IDENTIFIER_TOKEN droptable\n"
"IDENTIFIER_TOKEN:droptable *END*\n"

"SHOW TABLES\n"
"TABLES *END*\n"

"SELECT IDENTIFIER_TOKEN select\n"
"IDENTIFIER_TOKEN:select SPLICE\n"
"IDENTIFIER_TOKEN:select FROM\n"
"SPLICE IDENTIFIER_TOKEN select\n"
"FROM IDENTIFIER_TOKEN tableselect\n"
"IDENTIFIER_TOKEN:tableselect LIMIT tableselect\n"
"IDENTIFIER_TOKEN:tableselect WHERE tableselect\n"
"IDENTIFIER_TOKEN:tableselect ORDER\n"
"ORDER BY tableorderby\n"
"BY:tableorderby IDENTIFIER_TOKEN orderby\n"
"IDENTIFIER_TOKEN:orderby *END*\n"

"IDENTIFIER_TOKEN:orderby LIMIT tableselect\n"
"IDENTIFIER_TOKEN:orderby SPLICE orderbycomma\n"
"IDENTIFIER_TOKEN:orderby ASC orderby\n"
"IDENTIFIER_TOKEN:orderby DESC orderby\n"

"ASC:orderby SPLICE orderbycomma\n"
"ASC:orderby LIMIT tableselect\n"
"ASC:orderby *END*\n"
"DESC:orderby SPLICE orderbycomma\n"
"DESC:orderby LIMIT tableselect\n"
"DESC:orderby *END*\n"

"SPLICE:orderbycomma IDENTIFIER_TOKEN orderby\n"
"IDENTIFIER_TOKEN:tableselect *END*\n"
"IDENTIFIER_TOKEN:tableselect GROUP whereselect2\n"
"WHERE:tableselect IDENTIFIER_TOKEN whereselect\n"
"IDENTIFIER_TOKEN:whereselect EQUAL whereselect\n"
"IDENTIFIER_TOKEN:whereselect OPERATOR whereselect\n"
"EQUAL:whereselect IDENTIFIER_TOKEN whereselect2\n"
"OPERATOR:whereselect IDENTIFIER_TOKEN whereselect2\n"
"IDENTIFIER_TOKEN:whereselect2 GROUP whereselect2\n"
"GROUP:whereselect2 BY whereselect2\n"
"BY:whereselect2 IDENTIFIER_TOKEN groupbyselect\n"
"IDENTIFIER_TOKEN:groupbyselect *END*\n"
"IDENTIFIER_TOKEN:groupbyselect SPLICE groupbyselect\n"
"SPLICE:groupbyselect IDENTIFIER_TOKEN groupbyselect\n"
"IDENTIFIER_TOKEN:groupbyselect LIMIT tableselect\n"

"IDENTIFIER_TOKEN:whereselect2 *END*\n"
"IDENTIFIER_TOKEN:whereselect2 LIMIT tableselect\n"
"LIMIT:tableselect IDENTIFIER_TOKEN limit_tableselect\n"
"IDENTIFIER_TOKEN:limit_tableselect *END*\n"

"INSERT INTO\n"
"INTO IDENTIFIER_TOKEN tableinsert\n"
"IDENTIFIER_TOKEN:tableinsert LEFTP tableinsert\n"
"LEFTP:tableinsert IDENTIFIER_TOKEN tableinsertcolname\n"
"IDENTIFIER_TOKEN:tableinsertcolname SPLICE tableinsertcolname\n"
"SPLICE:tableinsertcolname IDENTIFIER_TOKEN tableinsertcolname\n"
"IDENTIFIER_TOKEN:tableinsertcolname RIGHTP tableinsertcolname\n"
"RIGHTP:tableinsertcolname VALUES\n"
"VALUES LEFTP tableinsert_v\n"
"LEFTP:tableinsert_v IDENTIFIER_TOKEN tableinsert_v\n"
"IDENTIFIER_TOKEN:tableinsert_v RIGHTP tableinsert_v\n"
"IDENTIFIER_TOKEN:tableinsert_v SPLICE tableinsert_v\n"
"SPLICE:tableinsert_v IDENTIFIER_TOKEN tableinsert_v\n"
"RIGHTP:tableinsert_v *END*\n"
"RIGHTP:tableinsert_v SPLICE tableinsert_new\n"
"SPLICE:tableinsert_new LEFTP tableinsert_v\n"


"DESCRIBE IDENTIFIER_TOKEN describe\n"
"IDENTIFIER_TOKEN:describe *END*\n"

"UPDATE IDENTIFIER_TOKEN tableupdate\n"
"IDENTIFIER_TOKEN:tableupdate SET\n"
"SET IDENTIFIER_TOKEN tableupdate_col\n"
"IDENTIFIER_TOKEN:tableupdate_col EQUAL tableupdate_val\n"
"EQUAL:tableupdate_val IDENTIFIER_TOKEN tableupdate_val\n"
"IDENTIFIER_TOKEN:tableupdate_val *END*\n"
"IDENTIFIER_TOKEN:tableupdate_val SPLICE tableupdate_val\n"
"SPLICE:tableupdate_val IDENTIFIER_TOKEN tableupdate_col\n"
"IDENTIFIER_TOKEN:tableupdate_val WHERE tableupdate\n"
"WHERE:tableupdate IDENTIFIER_TOKEN tableupdatef_col\n"
"IDENTIFIER_TOKEN:tableupdatef_col EQUAL tableupdatef_col\n"
"EQUAL:tableupdatef_col IDENTIFIER_TOKEN tableupdatef_val\n"
"IDENTIFIER_TOKEN:tableupdatef_val *END*\n"
"";

void setTableName(SqlQuery& query, LexTokens* token){
  auto identifier = std::get_if<IdentifierToken>(token);
  assert(identifier != NULL);
  query.table = identifier -> content;
}

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
  {"IDENTIFIER_TOKEN:table", setTableName},
  {"IDENTIFIER_TOKEN:create_tablecol", [](SqlQuery& query, LexTokens* token) -> void {
      auto identifier = std::get_if<IdentifierToken>(token);
      assert(identifier != NULL);
      auto createQuery = std::get_if<SqlCreate>(&query.queryData);
      assert(createQuery != NULL);
      createQuery -> columns.push_back(identifier -> content);
  }},
  {"IDENTIFIER_TOKEN:droptable", setTableName},
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
  {"IDENTIFIER_TOKEN:tableselect", setTableName},
  {"IDENTIFIER_TOKEN:limit_tableselect", [](SqlQuery& query, LexTokens* token) -> void {
      auto identifierToken = std::get_if<IdentifierToken>(token);
      assert(identifierToken != NULL);
      SqlSelect* selectQuery = std::get_if<SqlSelect>(&query.queryData);
      assert(selectQuery != NULL);
      selectQuery -> limit = std::atoi(identifierToken -> content.c_str()); // strong typing should occur earlier
  }},
  {"EQUAL:whereselect", [](SqlQuery& query, LexTokens* token) -> void {
      SqlSelect* selectQuery = std::get_if<SqlSelect>(&query.queryData);
      assert(selectQuery != NULL);
      selectQuery -> filter.hasFilter = true;
      selectQuery -> filter.type = EQUAL;
  }},
  {"OPERATOR:whereselect", [](SqlQuery& query, LexTokens* token) -> void {
      auto operatorToken = std::get_if<OperatorToken>(token);
      assert(operatorToken != NULL);
      SqlSelect* selectQuery = std::get_if<SqlSelect>(&query.queryData);
      assert(selectQuery != NULL);
      selectQuery -> filter.hasFilter = true;
      selectQuery -> filter.type = operatorToken -> type;
  }},
  {"IDENTIFIER_TOKEN:whereselect", [](SqlQuery& query, LexTokens* token) -> void {
      auto identifierToken = std::get_if<IdentifierToken>(token);
      assert(identifierToken != NULL);
      SqlSelect* selectQuery = std::get_if<SqlSelect>(&query.queryData);
      assert(selectQuery != NULL);
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
  {"IDENTIFIER_TOKEN:groupbyselect", [](SqlQuery& query, LexTokens* token) -> void {
    auto identifierToken = std::get_if<IdentifierToken>(token);
    assert(identifierToken != NULL);
    SqlSelect* selectQuery = std::get_if<SqlSelect>(&query.queryData);
    assert(selectQuery != NULL);
    selectQuery -> groupby.push_back(identifierToken -> content);
  }},
  {"IDENTIFIER_TOKEN:orderby", [](SqlQuery& query, LexTokens* token) -> void {
    auto identifierToken = std::get_if<IdentifierToken>(token);
    assert(identifierToken != NULL);
    SqlSelect* selectQuery = std::get_if<SqlSelect>(&query.queryData);
    assert(selectQuery != NULL);
    selectQuery -> orderBy.cols.push_back(identifierToken -> content);
    selectQuery -> orderBy.isDesc.push_back(false);
  }},
  {"DESC:orderby", [](SqlQuery& query, LexTokens* token) -> void {
    SqlSelect* selectQuery = std::get_if<SqlSelect>(&query.queryData);
    assert(selectQuery != NULL);
    auto lastIndex = selectQuery -> orderBy.isDesc.size() - 1;
    selectQuery -> orderBy.isDesc.at(lastIndex) = true;
  }},
  {"DESCRIBE", [](SqlQuery& query, LexTokens* token) -> void {
      query.type = SQL_DESCRIBE;
      query.queryData = SqlDescribe{};
  }},
  {"IDENTIFIER_TOKEN:describe", setTableName},
  {"INSERT", [](SqlQuery& query, LexTokens* token) -> void {
      query.type = SQL_INSERT;
      query.queryData = SqlInsert{};
  }},

  {"IDENTIFIER_TOKEN:tableinsert", setTableName},
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
      bool hasSpace = true;
      auto hasValue = insertQuery -> values.size() > 0;
      if (!hasValue){
        hasSpace = false;
      }else{
        auto lastValue = insertQuery -> values.at(insertQuery -> values.size() - 1);
        hasSpace = lastValue.size() <= insertQuery -> columns.size() - 1;       
      }
      if (!hasSpace){
        std::vector<std::string> value;
        insertQuery -> values.push_back(value);
      }
      insertQuery -> values.at(insertQuery -> values.size() - 1).push_back(identifierToken -> content);
  }},

  {"IDENTIFIER_TOKEN:tableupdate", [](SqlQuery& query, LexTokens* token) -> void {
    auto identifierToken = std::get_if<IdentifierToken>(token);
    assert(identifierToken != NULL);
    query.table = identifierToken -> content;
    query.type = SQL_UPDATE;
    query.queryData = SqlUpdate{};
  }},
  {"IDENTIFIER_TOKEN:tableupdate_col", [](SqlQuery& query, LexTokens* token) -> void {
    auto updateQuery = std::get_if<SqlUpdate>(&query.queryData);
    assert(updateQuery != NULL);
    auto identifierToken = std::get_if<IdentifierToken>(token);
    assert(identifierToken != NULL);
    updateQuery -> columns.push_back(identifierToken -> content);   
  }},
  {"IDENTIFIER_TOKEN:tableupdate_val", [](SqlQuery& query, LexTokens* token) -> void {
    auto updateQuery = std::get_if<SqlUpdate>(&query.queryData);
    assert(updateQuery != NULL);
    auto identifierToken = std::get_if<IdentifierToken>(token);
    assert(identifierToken != NULL);
    updateQuery -> values.push_back(identifierToken -> content);   
  }},
  {"IDENTIFIER_TOKEN:tableupdatef_col", [](SqlQuery& query, LexTokens* token) -> void {
    auto updateQuery = std::get_if<SqlUpdate>(&query.queryData);
    assert(updateQuery != NULL);
    auto identifierToken = std::get_if<IdentifierToken>(token);
    assert(identifierToken != NULL);
    updateQuery -> values.push_back(identifierToken -> content);   
    updateQuery -> filter.hasFilter = true;
    updateQuery -> filter.column = identifierToken -> content;
  }},
  {"IDENTIFIER_TOKEN:tableupdatef_val", [](SqlQuery& query, LexTokens* token) -> void {
    auto updateQuery = std::get_if<SqlUpdate>(&query.queryData);
    assert(updateQuery != NULL);
    auto identifierToken = std::get_if<IdentifierToken>(token);
    assert(identifierToken != NULL);
    updateQuery -> values.push_back(identifierToken -> content);   
    updateQuery -> filter.hasFilter = true;
    updateQuery -> filter.value = identifierToken -> content;
  }},

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