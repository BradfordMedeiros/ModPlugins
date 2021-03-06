#include "./sql.h"

std::string escapeCSVEntry(std::string data){
  // maybe just ban commas and newlines for now? 
  return data;
}

std::string qualifyColumnName(std::string tablename, std::string columnname){
  auto parts = split(columnname, '.');
  auto isQualified = parts.size() == 2;
  assert(parts.size() == 1 || parts.size() == 2);
  if (isQualified){
    return columnname;
  }
  return tablename + "." + columnname;
}
std::string dequalifyColumnName(std::string columnname){
  auto parts = split(columnname, '.');
  auto isQualified = parts.size() == 2;
  assert(parts.size() == 1 || parts.size() == 2);
  if (isQualified){
    return parts.at(1);
  }
  return parts.at(0);
}
std::string tableFromQualifiedName(std::string columnname){
  auto parts = split(columnname, '.');
  auto isQualified = parts.size() == 2;
  assert(parts.size() == 1 || parts.size() == 2);
  if (!isQualified){
    return "";
  }  
  return parts.at(0);
}

std::string tablePath(std::string tableName, std::string basePath){
  return basePath + tableName + ".csv";  // TODO do paths better bro
}

std::string createHeader(std::vector<std::string> columns){
  return join(columns, ',') + "\n";
}

bool isValidColumnName(std::string columnname){
  return (columnname.find('.') == std::string::npos) && (columnname.find(',') == std::string::npos) && (columnname.find('\n') == std::string::npos);
}
void createTable(std::string tableName, std::vector<std::string> columns, std::string basePath){
  auto filepath = tablePath(tableName, basePath);
  for (auto column : columns){
    assert(isValidColumnName(column));
  }
  saveFile(filepath, createHeader(columns));
}

void deleteTable(std::string tableName, std::string basePath){
  auto filepath = tablePath(tableName, basePath);
  std::remove(filepath.c_str());
}

struct TableData {
  std::vector<std::string> header;
  std::vector<std::vector<std::string>> rows;
};

TableData readTableData(std::string tableName, std::string basePath){
  auto tableContent = loadFile(tablePath(tableName, basePath));
  auto rawRows = filterWhitespace(split(tableContent, '\n'));
  auto header = split(rawRows.at(0), ',');
  std::vector<std::string> qualifiedHeader;
  for (auto col : header){
    qualifiedHeader.push_back(qualifyColumnName(tableName, col));
  }

  std::vector<std::vector<std::string>> rows;
  for (int i = 1; i < rawRows.size(); i++){
    auto columnContent = split(rawRows.at(i), ',');
    rows.push_back(columnContent);
    std::cout << "columns size: " << columnContent.size() << std::endl;
    std::cout << "header size: " << header.size() << std::endl;
    assert(columnContent.size() == header.size());
  }

  return TableData{
    .header = qualifiedHeader,
    .rows = rows,
  };
}
std::vector<std::string> readHeader(std::string tableName, std::string basePath){
  return readTableData(tableName, basePath).header;
}

std::vector<std::vector<std::string>> describeTable(std::string tableName, std::string basePath){
  std::vector<std::vector<std::string>> rows;
  for (auto header : readHeader(tableName, basePath)){
    rows.push_back({ dequalifyColumnName(header) });
  }
  return rows;
}
std::vector<std::vector<std::string>> showTables(std::string basePath){
  auto allFiles = listAllCsvFilesStems(basePath);
  std::vector<std::vector<std::string>> files;
  for (auto file : allFiles){
    files.push_back({ file });
  }
  return files;
}

void printColumns(std::vector<std::string> header){
  for (auto col : header){
    std::cout << col << " ";
  }
  std::cout << std::endl;
}

std::vector<int> getColumnIndexs(std::vector<std::string> header, std::vector<std::string> columns){
  std::vector<int> indexs;
  for (auto column : columns){
    bool foundCol = false;
    for (int i = 0; i < header.size(); i++){
      if (header.at(i) == column){
        indexs.push_back(i);
        foundCol = true;
      }
    }
    if (!foundCol){
      std::cout << "could not find col: " << column << std::endl;
      std::cout << "Header: ";
      printColumns(header);
      assert(foundCol);
    }
  }
  return indexs;
}

std::vector<int> getColumnsStarSelection(std::vector<std::string> header, std::vector<std::string> columns){
  std::vector<std::string> modifiedCols;
  for (auto column : columns){
    if (column == "*"){
      for (auto theColumn : header){
        bool columnAlreadyAdded = std::count(modifiedCols.begin(), modifiedCols.end(), theColumn) > 0;
        if (!columnAlreadyAdded){
          modifiedCols.push_back(theColumn);
        }
      }
      break;
    }else{
      modifiedCols.push_back(column);
    }
  }
  return getColumnIndexs(header, modifiedCols);
}

bool passesFilter(std::string& columnValue, SqlFilter& filter){
  if (filter.type == EQUAL){
    return columnValue == filter.value;
  }
  if (filter.type == NOT_EQUAL){
    return columnValue != filter.value;
  }
  if (filter.type == GREATER_THAN){
    return columnValue > filter.value;
  }
  if (filter.type == GREATER_THAN_OR_EQUAL){
    return columnValue >= filter.value;
  }
  if (filter.type == LESS_THAN){
    return columnValue < filter.value;
  }
  if (filter.type == LESS_THAN_OR_EQUAL){
    return columnValue <= filter.value;
  }
  std::cout << "operator not supported" << std::endl;
  assert(false);
  return false;
}

struct GroupingKey {
  std::vector<std::string> values;
};

bool groupingKeysEqual(GroupingKey& key1, GroupingKey& key2){
  assert(key1.values.size() == key2.values.size());
  for (int i = 0; i < key1.values.size(); i++){
    if (key1.values.at(i) != key2.values.at(i)){
      return false;
    }
  }
  return true;
}
GroupingKey createGroupingKey(std::vector<std::string>& row, std::vector<int>& indexs){
  std::vector<std::string> values;
  for (auto index : indexs){
    values.push_back(row.at(index));
  }
  GroupingKey key { .values = values };
  return key;
}
std::string hashGroupingKey(GroupingKey& key){
  std::string hash = "";
  for (auto value : key.values){
    hash = hash + value + ",";
  }
  return hash;
}

std::vector<std::string> fullQualifiedNames(std::string tablename, std::vector<std::string>& columns){
  std::vector<std::string> names;
  for (auto col : columns){
    names.push_back(qualifyColumnName(tablename, col));
  }
  return names;
}

struct JoinColumns {
  std::string table1Col;
  std::string table2Col;
};

bool columnNameInHeader(std::vector<std::string> header, std::string qualifiedColname){
  for (auto col : header){
    if (qualifiedColname == col){
      return true;
    }
  }
  return false;
}

std::string qualifyNameOrLeaveQualified(std::string table, std::string col1){
  // if is qualified leave
  if (tableFromQualifiedName(col1) != ""){
    return col1;
  }
  return qualifyColumnName(table, col1);
}


bool assignQualifiedColIfBelongs(std::string table, TableData& data, std::string col, std::string* outValue){
  auto colTable = qualifyNameOrLeaveQualified(table, col);
  for (auto col : data.header){
    if (col == colTable){
      *outValue = colTable;
      return true;
    }
  }
  return false;
}

JoinColumns figureOutJoinCols(std::string table1, TableData& data1, std::string table2, TableData& data2, std::string col1, std::string col2){
  std::string table1Col = "";
  std::string table2Col = "";

  auto col1IsTable1 = assignQualifiedColIfBelongs(table1, data1, col1, &table1Col);
  auto col1IsTable2 = assignQualifiedColIfBelongs(table2, data2, col1, &table2Col);

  auto col2IsTable1 = assignQualifiedColIfBelongs(table1, data1, col2, &table1Col);
  auto col2IsTable2 = assignQualifiedColIfBelongs(table2, data2, col2, &table2Col);

  assert(table1Col != "");
  assert(table2Col != "");

  return JoinColumns {
    .table1Col = table1Col,
    .table2Col = table2Col,
  };
}

TableData joinTableData(std::string table1, TableData& data1, std::string table2, TableData& data2, std::string col1, std::string col2, OperatorType op){
  auto joinCols = figureOutJoinCols(table1, data1, table2, data2, col1, col2);
  auto columnIndex1 = getColumnIndexs(data1.header,  { joinCols.table1Col }).at(0);
  auto columnIndex2 = getColumnIndexs(data2.header,  { joinCols.table2Col }).at(0);

  std::vector<std::string> header;
  for (auto col : fullQualifiedNames(table1, data1.header)){
    header.push_back(col);
  }
  for (auto col : fullQualifiedNames(table2, data2.header)){
    header.push_back(col);
  }

  std::vector<std::vector<std::string>> rows;
  for (int i = 0; i < data1.rows.size(); i++){
    bool hasMatch = false;
    auto row2Length = data2.rows.at(0).size();
    for (int j = 0; j < data2.rows.size(); j++){
      auto colOneValue = data1.rows.at(i).at(columnIndex1);
      auto colTwoValue = data2.rows.at(j).at(columnIndex2);
      auto matches =  colOneValue == colTwoValue;
      
      //std::cout << "matches? : " << matches << std::endl;
      //std::cout <<" Comparing: (" << col1 << " - " << col2 << ") -> " << colOneValue << " = " << colTwoValue << std::endl; 

      if (matches){
        hasMatch = true;
      }
      if (!matches){
        continue;
      }
      std::vector<std::string> row;
      for (auto colValue : data1.rows.at(i)){
        row.push_back(colValue);
      }
      for (auto colValue : data2.rows.at(j)){
        row.push_back(colValue);
      }
      rows.push_back(row);
    }
    if (!hasMatch){
      std::vector<std::string> row;
      for (auto colValue : data1.rows.at(i)){
        row.push_back(colValue);
      }
      for (int j = 0; j < row2Length; j++){
        row.push_back("NULL");
      }
      rows.push_back(row);
    }
  }

  return TableData {
    .header = header,
    .rows = rows,
  };
}



std::vector<std::vector<std::string>> select(std::string tableName, std::vector<std::string> columns, SqlJoin join, SqlFilter filter, SqlOrderBy orderBy, std::vector<std::string> groupBy, int limit, int offset, std::string basePath){
  auto tableData = readTableData(tableName, basePath);

  if (join.hasJoin){
    if (tableName == join.table){
      std::cout << "cannot join a table on itself" << std::endl;
      assert(false);
    }
    TableData additionalTableData = readTableData(join.table, basePath);
    std::cout << "main table: " << tableName << " join table " << join.table << std::endl;
    tableData = joinTableData(tableName, tableData, join.table, additionalTableData, join.col1, join.col2, join.type);
  }

  auto qualifiedColumns = fullQualifiedNames(tableName, columns);
  auto qualifiedOrderBy = fullQualifiedNames(tableName, orderBy.cols);
  auto qualifiedGroupBy = fullQualifiedNames(tableName, groupBy);

  std::vector<std::string> qualifiedFilter;
  if (filter.hasFilter){
    std::vector<std::string> filterColumns = { filter.column };
    auto qualedNames = fullQualifiedNames(tableName, filterColumns); 
    qualifiedFilter = qualedNames;
  }


  auto orderIndexs = getColumnIndexs(tableData.header, qualifiedOrderBy);
  std::sort (tableData.rows.begin(), tableData.rows.end(), [&orderIndexs, &orderBy](std::vector<std::string>& row1, std::vector<std::string>& row2) -> bool {
    for (int i = 0; i < orderIndexs.size(); i++){
      auto index = orderIndexs.at(i);
      auto value = strcmp(row1.at(index).c_str(), row2.at(index).c_str()); // this is wrong because row is already the new one 
      
      auto isDesc = orderBy.isDesc.at(i);
      if (value > 0){
        return isDesc ? true : false;
      }else if (value < 0){
        return isDesc ? false : true;
      }
    }
    return false;
  });

  std::vector<std::vector<std::string>> finalRows;
  auto filterIndex = -1;
  if (filter.hasFilter){
    filterIndex = getColumnIndexs(tableData.header, qualifiedFilter).at(0);
  }

  auto indexs = getColumnsStarSelection(tableData.header, qualifiedColumns);
  auto groupingIndexs = getColumnIndexs(tableData.header, qualifiedGroupBy);
  std::set<std::string> groupingKeysHash;

  for (int i = offset; i < tableData.rows.size(); i++){
    auto row = tableData.rows.at(i);
    if (filter.hasFilter){
      auto columnValue = row.at(filterIndex);
      auto passFilter = passesFilter(columnValue, filter);
      if (!passFilter){
        continue;
      }
    }
    if (limit >= 0 && finalRows.size() >= limit){
      break;
    }

    std::vector<std::string> organizedRow;

    if (groupBy.size() > 0){
      auto groupingKey = createGroupingKey(row, groupingIndexs);
      auto groupKeyHash = hashGroupingKey(groupingKey);
      auto alreadyHasKey = groupingKeysHash.find(groupKeyHash) != groupingKeysHash.end();
      if (!alreadyHasKey){
        groupingKeysHash.insert(groupKeyHash);
        for (auto index : indexs){
          organizedRow.push_back(row.at(index));
        }
        finalRows.push_back(organizedRow);
      }
    }else{
      for (auto index : indexs){
        organizedRow.push_back(row.at(index));
      }   
      finalRows.push_back(organizedRow);
    }
  }

  return finalRows;
}


std::string findValue(std::string columnToFind, std::vector<std::string>& columns, std::vector<std::string>& values){
  for (int i = 0; i < columns.size(); i++){
    if (columnToFind == columns.at(i)){
      return values.at(i);
    }
  }
  return "NULL";
}

std::string createRow(std::vector<std::string> values){
  return join(values, ',') + "\n";
}

void insert(std::string tableName, std::vector<std::string> columns, std::vector<std::vector<std::string>> values, std::string basePath){
  auto header = readHeader(tableName, basePath);
  auto indexs = getColumnIndexs(header, columns);

  std::string newContent = "";
  for (auto value : values){
    std::vector<std::string> valuesToInsert;
    for (int i = 0; i < header.size(); i++){
      assert(columns.size() == value.size());
      valuesToInsert.push_back(findValue(header.at(i), columns, value));
    }
    newContent = newContent + createRow(valuesToInsert);
  }
  appendFile(tablePath(tableName, basePath), newContent);
}

void update(std::string tableName, std::vector<std::string>& columns, std::vector<std::string>& values, std::string basePath){
  auto header = readHeader(tableName, basePath);
  auto allRows = select(tableName, header, {}, SqlFilter{ .hasFilter = false }, SqlOrderBy{}, {}, -1, 0, basePath);

  std::string content = createHeader(header);
  for (auto row : allRows){
    if (false){ // this is wrong
      content = content + "this one should be updated\n";
    }else{
      content = content + createRow(row);
    }
  }
  saveFile(tablePath(tableName, basePath), content);
}

void deleteRows(std::string tableName, SqlFilter& filter, std::string basePath){
  auto header = readHeader(tableName, basePath);
  auto rowsToKeep = select(tableName, header, {}, SqlFilter{}, SqlOrderBy{}, {}, -1, 0, basePath);
  std::string content = createHeader(header);
  for (auto row : rowsToKeep){
    if (filter.hasFilter){
      auto filterIndex = getColumnIndexs(header, { filter.column }).at(0);
      auto column = row.at(filterIndex);
      auto passFilter = passesFilter(column, filter);
      if (passFilter){
        continue;
      }
    }
    content = content + createRow(row);
  }
  saveFile(tablePath(tableName, basePath), content);
}

std::vector<std::vector<std::string>> executeSqlQuery(SqlQuery& query, std::string dataDir){
  assert(query.validQuery);
  if (query.type == SQL_SELECT){
    auto selectData = std::get_if<SqlSelect>(&query.queryData);
    assert(selectData != NULL);
    return select(query.table, selectData -> columns, selectData -> join, selectData -> filter, selectData -> orderBy, selectData -> groupby, selectData -> limit, selectData -> offset, dataDir);
  }else if (query.type == SQL_INSERT){
    auto insertData = std::get_if<SqlInsert>(&query.queryData);
    assert(insertData != NULL);
    insert(query.table, insertData -> columns, insertData -> values, dataDir);
    return {};
  }else if (query.type == SQL_UPDATE){
    auto updateData = std::get_if<SqlUpdate>(&query.queryData);
    assert(updateData != NULL);
    update(query.table, updateData -> columns, updateData -> values, dataDir);
    return {};
  }else if (query.type == SQL_DELETE){
    auto deleteData = std::get_if<SqlDelete>(&query.queryData);
    assert(deleteData != NULL);
    deleteRows(query.table, deleteData -> filter, dataDir);
    return {};
  }else if (query.type == SQL_CREATE_TABLE){
    auto createData = std::get_if<SqlCreate>(&query.queryData);
    assert(createData != NULL);
    createTable(query.table, createData -> columns, dataDir);
    return {};
  }else if (query.type == SQL_DELETE_TABLE){
    deleteTable(query.table, dataDir);
    return {};
  }else if (query.type == SQL_DESCRIBE){
    return describeTable(query.table, dataDir);
  }else if (query.type == SQL_SHOW_TABLES){
    return showTables(dataDir);
  }
  assert(false);
  return {};
}
