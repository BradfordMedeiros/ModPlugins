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

std::vector<std::vector<std::string>> select(std::string tableName, std::vector<std::string> columns, SqlFilter filter, SqlOrderBy orderBy, int limit){
  auto tableData = readTableData(tableName);
  std::vector<std::vector<std::string>> rows;

  auto filterIndex = -1;
  if (filter.hasFilter){
    filterIndex = getColumnIndexs(tableData.header, { filter.column }).at(0);
  }

  auto indexs = getColumnIndexs(tableData.header, columns);
  for (int i = 1; i < tableData.rawRows.size(); i++){
    std::vector<std::string> row;
    auto columnContent = split(tableData.rawRows.at(i), ',');
    for (auto index : indexs){
      row.push_back(columnContent.at(index));
    }
    rows.push_back(row);
  }

  auto orderIndexs = getColumnIndexs(tableData.header, orderBy.cols);
  std::sort (rows.begin(), rows.end(), [&orderIndexs](std::vector<std::string>& row1, std::vector<std::string>& row2) -> bool {
    for (auto index : orderIndexs){
      auto value = strcmp(row1.at(index).c_str(), row2.at(index).c_str()); // this is wrong because row is already the new one 
      if (value > 0){
        return false;
      }else if (value < 0){
        return true;
      }
    }
    return false;
  });

  std::vector<std::vector<std::string>> finalRows;
  for (auto row : rows){
    if (filter.hasFilter){
      auto columnValue = row.at(filterIndex);
      if (filter.type == EQUAL){
        if (columnValue != filter.value){
          continue;
        }
      }else if (filter.type == NOT_EQUAL){
        if (columnValue == filter.value){
          continue;
        }
      }else if (filter.type == GREATER_THAN){
        if (columnValue <= filter.value){
          continue;
        }
      }else if (filter.type == GREATER_THAN_OR_EQUAL){
        if (columnValue < filter.value){
          continue;
        }
      }else if (filter.type == LESS_THAN){
        if (columnValue >= filter.value){
          continue;
        }
      }else if (filter.type == LESS_THAN_OR_EQUAL){
        if (columnValue > filter.value){
          continue;
        }
      }else{
        std::cout << "operator not supported" << std::endl;
        assert(false);
      }
 
    }
    if (limit >= 0 && rows.size() >= limit){
      break;
    }
    finalRows.push_back(row);
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
  auto allRows = select(tableName, tableData.header, SqlFilter{ .hasFilter = false }, SqlOrderBy{}, -1);

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

OperatorType oppositeFilter(OperatorType filterType){
  if (filterType == EQUAL){
    return NOT_EQUAL;
  }
  if (filterType == NOT_EQUAL){
    return EQUAL;
  }
  if (filterType == GREATER_THAN){
    return LESS_THAN_OR_EQUAL;
  }
  if (filterType == LESS_THAN){
    return GREATER_THAN_OR_EQUAL;
  }
  if (filterType == GREATER_THAN_OR_EQUAL){
    return LESS_THAN;
  }
  if (filterType == LESS_THAN_OR_EQUAL){
    return GREATER_THAN;
  }
  assert(false);
  return EQUAL;
}

void deleteRows(std::string tableName, SqlFilter& filter){
  assert(filter.hasFilter);

  auto tableData = readTableData(tableName);
  auto copyFilter = filter;
  copyFilter.type = oppositeFilter(filter.type);

  auto rowsToKeep = select(tableName, tableData.header, copyFilter, SqlOrderBy{}, -1);

  std::string content = createHeader(tableData.header);
  for (auto row : rowsToKeep){
    content = content + createRow(row);
  }
  saveFile(tablePath(tableName), content);
}

std::vector<std::vector<std::string>> executeSqlQuery(SqlQuery& query){
  assert(query.validQuery);
  if (query.type == SQL_SELECT){
    auto selectData = std::get_if<SqlSelect>(&query.queryData);
    assert(selectData != NULL);
    return select(query.table, selectData -> columns, selectData -> filter, selectData -> orderBy, selectData -> limit);
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
