#include <iostream>
#include <libguile.h>
#include <vector>
#include "./sequencer.h"

typedef int32_t (*func_i)();
func_i currentModuleId = NULL;

SCM trackType; // this is modified during init
struct scmTrack {
  Track track;
  std::vector<SCM> funcRefs;
};

scmTrack* getTrackFromScmType(SCM value){
  scmTrack* obj;
  scm_assert_foreign_object_type (trackType, value);
  obj = (scmTrack*)scm_foreign_object_ref(value, 0);
  return obj; 
}

void finalizeTrack (SCM trackobj){  // test bvy invoking [gc]
  auto track = getTrackFromScmType(trackobj);
  for (auto scmFn : track -> funcRefs){
    scm_gc_unprotect_object(scmFn);
  }
}

SCM scmCreateTrack(SCM name, SCM funcs){
  auto trackobj = (scmTrack*)scm_gc_malloc(sizeof(scmTrack), "track");

  std::vector<std::function<void()>> tracks;
  std::vector<SCM> funcRefs;

  auto numTrackFns = scm_to_int32(scm_length(funcs));
  for (int i = 0; i < numTrackFns; i++){
    SCM func = scm_list_ref(funcs, scm_from_unsigned_integer(i));  
    bool isThunk = scm_to_bool(scm_procedure_p(func));
    assert(isThunk);
    tracks.push_back([func]() -> void{
      scm_call_0(func);  
    });

    scm_gc_protect_object(func); // ref counting to prevent garbage collection, decrement happens in finalizer 
    funcRefs.push_back(func);
  }

  auto track = createTrack(scm_to_locale_string(name), tracks);
  trackobj -> track = track;
  trackobj -> funcRefs = funcRefs;

  return scm_make_foreign_object_1(trackType, trackobj);
}

SCM scmPlayTrack(SCM track){
  playbackTrack(getTrackFromScmType(track) -> track);
  return SCM_UNSPECIFIED;
}

SCM onExitType; // this is modified during init
struct scmOnExit {
  std::vector<SCM> funcRefs;
  ExitCallback onExit;
};
scmOnExit* getOnExitFromScmType(SCM value){
  scmOnExit* obj;
  scm_assert_foreign_object_type(onExitType, value);
  obj = (scmOnExit*)scm_foreign_object_ref(value, 0);
  return obj;
}
void finalizeOnExit(SCM onexit){
  auto onExit = getOnExitFromScmType(onexit);
  for (auto scmFn : onExit -> funcRefs){
    scm_gc_unprotect_object(scmFn);
  }
}

SCM scmCreateOnExit(SCM funcs){
  auto onexitObj = (scmOnExit*)scm_gc_malloc(sizeof(scmOnExit), "onexit");
  std::vector<SCM> funcRefs;
  std::vector<std::function<void()>> exitFns;

  auto numOnExitFns = scm_to_int32(scm_length(funcs));
  for (int i = 0; i < numOnExitFns; i++){
    SCM func = scm_list_ref(funcs, scm_from_unsigned_integer(i));  
    bool isThunk = scm_to_bool(scm_procedure_p(func));
    assert(isThunk);
    scm_gc_protect_object(func); 
    funcRefs.push_back(func);
    exitFns.push_back([func]() -> void {
      scm_call_0(func);  
    });
  }
  onexitObj -> funcRefs = funcRefs;

  ExitCallback onExit {
    .exitFns = exitFns,
  };
  onexitObj -> onExit = onExit;
  return scm_make_foreign_object_1(onExitType, onexitObj);
}

std::variant<scmTrack*, scmOnExit*> getStateType(SCM scmValue){
  auto valueClass = scm_class_of(scmValue);
  bool isTrack = scm_is_eq(valueClass, trackType);
  bool isOnExit = scm_is_eq(valueClass, onExitType);
  if (isTrack){
    scmTrack* track = getTrackFromScmType(scmValue);
    return track;   
  }
  if (isOnExit){
    scmOnExit* onExit = getOnExitFromScmType(scmValue);
    return onExit;
  }
  assert(false);
}

SCM stateType; // this is modified during init
SCM scmState(SCM name, SCM scmTracks){                  
  auto stateobj = (State*)scm_gc_malloc(sizeof(State), "state");
  std::map<std::string, std::string> attributes;
  std::map<std::string, Track> tracks;

  auto listLength = scm_to_int32(scm_length(scmTracks));

  std::string defaultTrack;
  int numTracks = 0;
  ExitCallback onExit{};

  for (unsigned int i = 0; i < listLength; i++){
    SCM trackValue = scm_list_ref(scmTracks, scm_from_int64(i));
    auto pvalue = getStateType(trackValue);
    auto ptrack = std::get_if<scmTrack*>(&pvalue);
    auto pOnExit = std::get_if<scmOnExit*>(&pvalue);

    if (ptrack != NULL){
      numTracks++;
      auto track = *ptrack;
      tracks[track -> track.name] = track -> track;  // @TODO - probably need to add extra track scm gc lock here + finalize, since making copy of the track
      if (numTracks == 1){
        defaultTrack = track -> track.name;
      }
    }else if (pOnExit != NULL){
      onExit = (*pOnExit) -> onExit;
    }else{
      assert(false);
    }
  }
  assert(listLength > 0);

  State state {
    .name = scm_to_locale_string(name),
    .defaultTrack = defaultTrack,
    .attributes = attributes,
    .tracks = tracks,
    .onExit = onExit,
  };
  *stateobj = state;
  return scm_make_foreign_object_1(stateType, stateobj);
}

std::vector<State> fromScmStateList(SCM statesList){
  std::vector<State> states;
  auto listLength = scm_to_int32(scm_length(statesList));
  for (unsigned int i = 0; i < listLength; i++){
    SCM scmValue = scm_list_ref(statesList, scm_from_int64(i));
    scm_assert_foreign_object_type(stateType, scmValue);
    State* obj = (State*)scm_foreign_object_ref(scmValue, 0);
    states.push_back(*obj);
  }
  return states;
}


SCM stateMachineType; // this is modified during init
StateMachine* getMachineFromScmType(SCM value){
  StateMachine* obj;
  scm_assert_foreign_object_type (stateMachineType, value);
  obj = (StateMachine*)scm_foreign_object_ref(value, 0);
  return obj; 
}

StateMachine (*_createStateMachine)(std::vector<State> states);
SCM scmStateMachine(SCM states){
  auto statemachineobj = (StateMachine*)scm_gc_malloc(sizeof(StateMachine), "statemachine");
  auto machine = _createStateMachine(fromScmStateList(states));
  *statemachineobj = machine;
  return scm_make_foreign_object_1(stateMachineType, statemachineobj);
}

SCM scmPlayStateMachine(SCM scmMachine){
  auto moduleId = currentModuleId();
  playStateMachine(getMachineFromScmType(scmMachine), moduleId);
  return SCM_UNSPECIFIED;
}

SCM scmSetStateMachine(SCM scmMachine, SCM state){
  setStateMachine(getMachineFromScmType(scmMachine), scm_to_locale_string(state));
  return SCM_UNSPECIFIED;
}

void registerGuileFns() asm ("registerGuileFns");
void registerGuileFns() { 
  scm_c_define_gsubr("create-track", 2, 0, 0, (void*)scmCreateTrack);
  scm_c_define_gsubr("play-track", 1, 0, 0, (void*)scmPlayTrack);
  scm_c_define_gsubr("on-exit", 1, 0, 0, (void*)scmCreateOnExit);
  scm_c_define_gsubr("state", 2, 0, 0, (void*)scmState);
  scm_c_define_gsubr("machine", 1, 0, 0, (void*)scmStateMachine);
  scm_c_define_gsubr("play-machine", 1, 0, 0, (void*)scmPlayStateMachine);
  scm_c_define_gsubr("set-machine", 2, 0, 0, (void*)scmSetStateMachine);
}

void registerGuileTypes() asm("registerGuileTypes");
void registerGuileTypes(){
  trackType = scm_make_foreign_object_type(scm_from_utf8_symbol("track"), scm_list_1(scm_from_utf8_symbol("data")), finalizeTrack);
  onExitType = scm_make_foreign_object_type(scm_from_utf8_symbol("onexit"), scm_list_1(scm_from_utf8_symbol("data")), finalizeOnExit);
  stateType = scm_make_foreign_object_type(scm_from_utf8_symbol("state"),  scm_list_1(scm_from_utf8_symbol("data")), NULL);
  stateMachineType = scm_make_foreign_object_type(scm_from_utf8_symbol("statemachine"),  scm_list_1(scm_from_utf8_symbol("data")), NULL);
}

void registerGetCurrentModule(func_i getCurrentModule) asm("registerGetCurrentModule");
void registerGetCurrentModule(func_i getCurrentModule){
  currentModuleId = getCurrentModule;
}

void onScriptUnload(int32_t scriptId) asm("onScriptUnload");
void onScriptUnload(int32_t scriptId){
  assert(scriptId == currentModuleId());
  removeStateMachines(scriptId);
}

void onFrame() asm ("onFrame");
void onFrame() { 
  processStateMachines();
}

#ifdef BINARY_MODE

int main(int argc, char *argv[]){
  std::cout << "hello world" << std::endl;

}

#endif