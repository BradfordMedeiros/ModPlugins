#include "./sql.h"

std::string escapeCSVEntry(std::string data){
  // maybe just ban commas and newlines for now? 
  return data;
}

std::string basePath = "./res/state/";
std::string tablePath(std::string tableName){
  return basePath + tableName + ".csv";  // TODO do paths better bro
}

std::string createHeader(std::vector<std::string> columns){
  return join(columns, ',') + "\n";
}

void createTable(std::string tableName, std::vector<std::string> columns){
  auto filepath = tablePath(tableName);
  saveFile(filepath, createHeader(columns));
}
void deleteTable(std::string tableName){
  auto filepath = tablePath(tableName);
  std::remove(filepath.c_str());
}

struct TableData {
  std::vector<std::string> header;
  std::vector<std::string> rawRows;
};
TableData readTableData(std::string tableName){
  auto tableContent = loadFile(tablePath(tableName));
  auto rawRows = split(tableContent, '\n');
  auto header = split(rawRows.at(0), ',');
  return TableData{
    .header = header,
    .rawRows = rawRows,
  };
}

std::vector<std::vector<std::string>> describeTable(std::string tableName){
  std::vector<std::vector<std::string>> rows;
  auto tableData = readTableData(tableName);
  for (auto header : tableData.header){
    rows.push_back({ header });
  }
  return rows;
}
std::vector<std::vector<std::string>> showTables(){
  auto allFiles = listAllCsvFilesStems(basePath);
  std::vector<std::vector<std::string>> files;
  for (auto file : allFiles){
    files.push_back({ file });
  }
  return files;
}

std::vector<int> getColumnIndexs(std::vector<std::string> header, std::vector<std::string> columns){
  std::vector<int> indexs;
  for (auto column : columns){
    bool foundCol = false;
    for (int i = 0; i < header.size(), !foundCol; i++){
      if (header.at(i) == column){
        indexs.push_back(i);
        foundCol = true;
      }
    }
    assert(foundCol);
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


std::vector<std::vector<std::string>> select(std::string tableName, std::vector<std::string> columns, SqlJoin join, SqlFilter filter, SqlOrderBy orderBy, std::vector<std::string> groupBy, int limit){
  auto tableData = readTableData(tableName);
  std::vector<std::vector<std::string>> rows;

  for (int i = 1; i < tableData.rawRows.size(); i++){
    auto columnContent = split(tableData.rawRows.at(i), ',');
    rows.push_back(columnContent);
  }

  auto orderIndexs = getColumnIndexs(tableData.header, orderBy.cols);
  std::sort (rows.begin(), rows.end(), [&orderIndexs, &orderBy](std::vector<std::string>& row1, std::vector<std::string>& row2) -> bool {
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
    filterIndex = getColumnIndexs(tableData.header, { filter.column }).at(0);
  }

  auto indexs = getColumnsStarSelection(tableData.header, columns);
  auto groupingIndexs = getColumnIndexs(tableData.header, groupBy);
  std::set<std::string> groupingKeysHash;

  for (auto row : rows){
    if (filter.hasFilter){
      auto columnValue = row.at(filterIndex);
      auto passFilter = passesFilter(columnValue, filter);
      if (!passFilter){
        continue;
      }
    }
    if (limit >= 0 && rows.size() >= limit){
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

void insert(std::string tableName, std::vector<std::string> columns, std::vector<std::vector<std::string>> values){
  auto header = readTableData(tableName).header;
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
  appendFile(tablePath(tableName), newContent);
}

void update(std::string tableName, std::vector<std::string>& columns, std::vector<std::string>& values){
  auto tableData = readTableData(tableName);
  auto allRows = select(tableName, tableData.header, {}, SqlFilter{ .hasFilter = false }, SqlOrderBy{}, {}, -1);

  std::string content = createHeader(tableData.header);
  for (auto row : allRows){
    if (false){ // this is wrong
      content = content + "this one should be updated\n";
    }else{
      content = content + createRow(row);
    }
  }
  saveFile(tablePath(tableName), content);
}

void deleteRows(std::string tableName, SqlFilter& filter){
  auto tableData = readTableData(tableName);
  auto rowsToKeep = select(tableName, tableData.header, {}, SqlFilter{}, SqlOrderBy{}, {}, -1);
  std::string content = createHeader(tableData.header);
  for (auto row : rowsToKeep){
    if (filter.hasFilter){
      auto filterIndex = getColumnIndexs(tableData.header, { filter.column }).at(0);
      auto column = row.at(filterIndex);
      auto passFilter = passesFilter(column, filter);
      if (passFilter){
        continue;
      }
    }
    content = content + createRow(row);
  }
  saveFile(tablePath(tableName), content);
}

std::vector<std::vector<std::string>> executeSqlQuery(SqlQuery& query){
  assert(query.validQuery);
  if (query.type == SQL_SELECT){
    auto selectData = std::get_if<SqlSelect>(&query.queryData);
    assert(selectData != NULL);
    return select(query.table, selectData -> columns, selectData -> join, selectData -> filter, selectData -> orderBy, selectData -> groupby, selectData -> limit);
  }else if (query.type == SQL_INSERT){
    auto insertData = std::get_if<SqlInsert>(&query.queryData);
    assert(insertData != NULL);
    insert(query.table, insertData -> columns, insertData -> values);
    return {};
  }else if (query.type == SQL_UPDATE){
    auto updateData = std::get_if<SqlUpdate>(&query.queryData);
    assert(updateData != NULL);
    update(query.table, updateData -> columns, updateData -> values);
    return {};
  }else if (query.type == SQL_DELETE){
    auto deleteData = std::get_if<SqlDelete>(&query.queryData);
    assert(deleteData != NULL);
    deleteRows(query.table, deleteData -> filter);
    return {};
  }else if (query.type == SQL_CREATE_TABLE){
    auto createData = std::get_if<SqlCreate>(&query.queryData);
    assert(createData != NULL);
    createTable(query.table, createData -> columns);
    return {};
  }else if (query.type == SQL_DELETE_TABLE){
    deleteTable(query.table);
    return {};
  }else if (query.type == SQL_DESCRIBE){
    return describeTable(query.table);
  }else if (query.type == SQL_SHOW_TABLES){
    return showTables();
  }
  assert(false);
  return {};
}
