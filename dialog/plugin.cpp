#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <assert.h>
#include <set>
#include <sstream>

const char* nodeTransitions = " "
  "start to1 middle\n"
  "middle to2 end1\n"
  "middle to3 end2\n" 
  "start gotoend end2\n"
"";

const char* nodeProperties = " "
  "start text this_is_some_cool_text_yay\n"
  "end text this more_cool_text\n"
  "start color red\n" 
  "end color blue\n"
  "loner_node color blue\n"
"";

struct Node {
  std::map<std::string, std::string> transitionNameToNode;
  std::map<std::string, std::string> properties;  // See note below
};  

struct NodeTransition {
  std::string nodeFrom;
  std::string transition;
  std::string nodeTo;
};

struct NodeProperty {
  std::string node;
  std::string key;
  std::string value;
};

struct DialogData {
  std::vector<NodeTransition> transitions;
  std::vector<NodeProperty> properties;
};

std::set<std::string> allNodeNames(DialogData& data){
  std::set<std::string> nodes;
  for (auto transitions : data.transitions){
    nodes.insert(transitions.nodeFrom);
    nodes.insert(transitions.nodeTo);
  }
  return nodes;
}

std::map<std::string, std::string> allTransitions(DialogData& data, std::string nodename){
  std::map<std::string, std::string> transitions;
  for (auto transition : data.transitions){
    if (transition.nodeFrom == nodename){
      transitions[transition.transition] = transition.nodeTo;
    }
  }
  return transitions;
}

std::map<std::string, std::string> allProperties(DialogData& data, std::string nodename){
  std::map<std::string, std::string> properties;
  for (auto property : data.properties){
    if (property.node == nodename){
      properties[property.key] = property.value;
    }
  }
  return properties;
}


// Common code copied pasta from common/util.h in modengine.  Maybe should use common.  But probably end up using sql. 
std::string trim(const std::string& str){
  size_t first = str.find_first_not_of(' ');
  if (std::string::npos == first){
    return str;
  }
  size_t last = str.find_last_not_of(' ');
  return str.substr(first, (last - first + 1));
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
std::vector<std::string> filterWhitespace(std::vector<std::string> values){
  std::vector<std::string> newStrings;
  for (auto value : values){
    auto newValue = trim(value);
    if (newValue != ""){
      newStrings.push_back(newValue);
    }
  }
  return newStrings;
}

std::vector<std::vector<std::string>> parseValues(std::string& data){
  std::vector<std::vector<std::string>> values;
  auto vecData = filterWhitespace(split(data, '\n'));
  for (auto data : vecData){
    values.push_back(filterWhitespace(split(data, ' ')));
  }
  return values;
}


std::vector<NodeTransition> deserializeTransitions(std::string& transitionsStr){
  std::vector<NodeTransition> transitions;
  auto values = parseValues(transitionsStr);
  for (auto value : values){
    assert(value.size() == 3);
    transitions.push_back(NodeTransition{
      .nodeFrom = value.at(0),
      .transition = value.at(1),
      .nodeTo = value.at(2),
    });
  }
  return transitions;
}

std::vector<NodeProperty> deserializeProperties(std::string& propertiesStr){
  std::vector<NodeProperty> properties;
  auto values = parseValues(propertiesStr);
  for (auto value : values){
    assert(value.size() >= 3);
    properties.push_back(NodeProperty{
      .node = value.at(0),
      .key = value.at(1),
      .value = value.at(2),
    });
  }
  return properties;
}


DialogData deserializeDialogData(std::string transitions, std::string properties){
  DialogData data {
    .transitions = deserializeTransitions(transitions),
    .properties = deserializeProperties(properties),
  };
  return data;
}

std::map<std::string, Node> createDialogTree(DialogData data){
  std::map<std::string, Node> dialogTree;
  for (auto nodename : allNodeNames(data)){
    assert(dialogTree.find(nodename) == dialogTree.end());
    dialogTree[nodename] = Node {
      .transitionNameToNode = allTransitions(data, nodename),
      .properties = allProperties(data, nodename),
    };
  }
  return dialogTree;
}

// looks like
//strict graph {
//"nodefrom" -- "nodeto"
//}

std::string getNodeInfo(std::string name, Node& node){
  return name;
}

void dumpDotFormat(std::map<std::string, Node>& dialogTree){
  std::string text = "strict graph {\n";
  for (auto [name, node] : dialogTree){
    for (auto [transitionName, transitionTo] : node.transitionNameToNode){
      text = text + 
        "\"" + getNodeInfo(name, node) + "\""  + 
        " -- " + 
        "\"" + getNodeInfo(transitionTo, dialogTree.at(transitionTo))  + "\"" +
        " [ label=\"" + transitionName + "\" ];" +
        "\n";
    } 
  }
  text = text + "}";
  std::cout << text << std::endl;
}

#ifdef BINARY_MODE

int main(){
  auto dialogTree = createDialogTree(deserializeDialogData(nodeTransitions, nodeProperties));
  dumpDotFormat(dialogTree);
}

#endif
