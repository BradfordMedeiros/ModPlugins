#include "./sequencer.h"

Track createTrack(std::string name, std::vector<std::function<void()>> fns){
  Track track {
    .name = name,
    .trackFns = fns,
  };
  return track;
}
void playbackTrack(Track& track){
  for (auto trackFn : track.trackFns){
    trackFn();
  }
}

StateMachine createStateMachine(std::vector<State> states){
  assert(states.size() > 0);
  std::map<std::string, State> stateMapping;
  for (auto state : states){
    stateMapping[state.name] = state;
  }
  StateMachine machine {
    .currentState = states.at(0).name,
    .currentTrack = states.at(0).tracks.at(states.at(0).defaultTrack).name,
    .trackIndex = 0,
    .states = stateMapping,
  };
  return machine;
}

std::map<unsigned int, std::vector<StateMachine*>> activeMachines;

void setStateMachine(StateMachine* machine, std::string newState){
  if (machine -> currentState == newState){
    return;
  }
  auto onExit = machine -> states.at(machine -> currentState).onExit;
  for (auto fn : onExit.exitFns){
    fn();
  }

  machine -> currentState = newState;
  machine -> currentTrack = machine -> states.at(newState).defaultTrack;
  machine -> trackIndex = 0;
}

void playStateMachine(StateMachine* machine, unsigned int id){
  if (activeMachines.find(id) == activeMachines.end()){
    activeMachines[id] = {};
  }
  activeMachines.at(id).push_back(machine);
}
void removeStateMachines(unsigned int id){
  activeMachines.erase(id);
}


void processStateMachines(){
  for (auto &[_, machinesForId] : activeMachines){
    for (auto machine : machinesForId){
      State& activeState = machine -> states.at(machine -> currentState);
      Track& currentTrack = activeState.tracks.at(machine -> currentTrack);
      for (int i = machine -> trackIndex; i < currentTrack.trackFns.size(); i++){
         auto fn = currentTrack.trackFns.at(i);
        fn();
        machine -> trackIndex++;
      }
    }
  }
} 