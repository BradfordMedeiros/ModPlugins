#include "./util.h"

/// warning copy/paste part. Maybe should include common code for such little code...?  But might be not worth it...
std::string join(std::vector<std::string> values, char delimeter){
  std::string value = "";
  for (int i = 0; i < values.size(); i++){
    value = value + values[i];
    if (i < (values.size() - 1)){
      value = value + delimeter;
    }
  }
  return value;
}
void saveFile(std::string filepath, std::string content){
  std::ofstream file;
  file.open(filepath);
  file << content;
  file.close();
}
std::string loadFile(std::string filepath){
   std::ifstream file(filepath.c_str());
   if (!file.good()){
     throw std::runtime_error("file not found" + filepath);
   }   
   std::stringstream buffer;
   buffer << file.rdbuf();
   return buffer.str();
}
std::vector<std::string> split(std::string strToSplit, char delimeter){
  std::stringstream ss(strToSplit);
  std::string item;
  std::vector<std::string> splittedStrings;
  while (std::getline(ss, item, delimeter)){
    splittedStrings.push_back(item);
  }
  return splittedStrings;
}
void appendFile(std::string filepath, std::string content){
  std::cout << "appending: " << filepath << " w/ " << content << std::endl;
  std::fstream file(filepath.c_str(), std::ios::out | std::ios::app);
  file << content;
  file.close();
}
/////////////////////