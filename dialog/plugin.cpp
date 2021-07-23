#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <assert.h>
#include <set>

const char* nodeTransitions = " "
  "start to1 middle"
  "middle to2 end1"
  "middle to3 end2" 
  "start to4 end2"
"";

const char* nodeProperties = " "
  "start text this is some cool text yay"
  "end text this is some more text wow cool"
  "start color red" 
  "end color blue"
  "loner_node color blue"
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

std::map<std::string, std::string> allTransitions(DialogData& data){
  std::map<std::string, std::string> transitions;
  return transitions;
}

std::map<std::string, std::string> allProperties(DialogData& data){
  std::map<std::string, std::string> properties;
  return properties;
}

DialogData deserializeDialogData(std::string transitions, std::string properties){
  DialogData data {

  };
  return data;
}

std::map<std::string, Node> createDialogTree(DialogData data){
  std::map<std::string, Node> dialogTree;
  for (auto nodename : allNodeNames(data)){
    assert(dialogTree.find(nodename) == dialogTree.end());
    dialogTree[nodename] = Node {
      .transitionNameToNode = allTransitions(data),
      .properties = allProperties(data),
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
    for (auto [_, transitionTo] : node.transitionNameToNode){
      text = text + "\"" + getNodeInfo(name, node) + "\""  + " -- " + "\"" + getNodeInfo(transitionTo, dialogTree.at(transitionTo))  + "\"";
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
