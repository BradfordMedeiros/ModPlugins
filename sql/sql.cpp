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
  std::vector<std::vector<std::string>> additionalOrderByRows;


  auto filterIndex = -1;
  if (filter.hasFilter){
    filterIndex = getColumnIndexs(tableData.header, { filter.column }).at(0);
  }

  auto indexs = getColumnIndexs(tableData.header, columns);

  auto orderByIndexs = getColumnIndexs(tableData.header, orderBy.cols);
  std::vector<int> additionalIndexs;
  for (auto orderIndex : orderByIndexs){
    bool indexAlreadyIncluded = false;
    for (auto index : indexs){
      if (orderIndex == index){
        indexAlreadyIncluded = true;
        break;
      }
    }
    if (!indexAlreadyIncluded){
      additionalIndexs.push_back(orderIndex);
    }
  }


  for (int i = 1; i < tableData.rawRows.size(); i++){
    std::vector<std::string> row;
    std::vector<std::string> orderByRow;
    auto columnContent = split(tableData.rawRows.at(i), ',');
    for (auto index : indexs){
      row.push_back(columnContent.at(index));
    }
    for (auto index : orderByIndexs){
      orderByRow.push_back(columnContent.at(index));
    }
    if (filter.hasFilter){
      auto columnValue = columnContent.at(filterIndex);
      if (!filter.invert && columnValue != filter.value){
        continue;
      }
      if (filter.invert && columnValue == filter.value){
        continue;
      }
    }
    if (limit >= 0 && rows.size() >= limit){
      break;
    }
    rows.push_back(row);
    additionalOrderByRows.push_back(orderByRow);
  }

  std::map<std::vector<std::string>*, std::vector<std::string>*> additionalMapping;
  for (int i = 0; i < rows.size(); i++){
    additionalMapping[&rows.at(i)] = &additionalOrderByRows.at(i);
  }

  if (orderBy.cols.size() > 0){
    std::cout << "ordering: ";
    for (auto col : orderBy.cols){
      std::cout << col << " ";
    }
    std::cout << std::endl;
    auto indexs = getColumnIndexs(tableData.header, orderBy.cols);


    // This is wrong, because the row1/2 are reordered, not the original indexs 
    // Additionally it doesn't even have the all the possible values that can be ordered by
    std::sort (rows.begin(), rows.end(), [&indexs](std::vector<std::string>& row1, std::vector<std::string>& row2) -> bool {
      std::cout << "sorting by: ";
      for (auto index : indexs){
        std::cout << index << " ";
        auto value = strcmp(row1.at(index).c_str(), row2.at(index).c_str()); // this is wrong because row is already the new one 
        if (value > 0){
          std::cout << std::endl;
          return false;
        }else if (value < 0){
          std::cout << std::endl;
          return true;
        }
      }
      std::cout << std::endl;
      return true;
    });
  }
  return rows;
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

void deleteRows(std::string tableName, SqlFilter& filter){
  assert(filter.hasFilter);

  auto tableData = readTableData(tableName);
  auto copyFilter = filter;
  copyFilter.invert = !filter.invert;

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
