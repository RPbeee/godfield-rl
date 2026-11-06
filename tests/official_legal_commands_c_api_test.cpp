#include "godfield/official_match_c_api.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
namespace {
void check(bool ok,const char* text){if(!ok){std::cerr<<text<<'\n';std::exit(1);}}
}
int main(){
    gfo_match* game=nullptr;
    gfo_match_player players[2]{};players[1].id=77;
    check(gfo_match_create(nullptr,players,2,42,&game)==0,"create");
    size_t n=999;
    check(gfo_match_legal_commands(game,2,0,nullptr,0,&n)==0&&n>0,"size query");
    const auto count=n;
    std::vector<gfo_match_command> buffer(n+2);
    std::memset(buffer.data(),0x5a,buffer.size()*sizeof(buffer[0]));const auto before=buffer;
    n=123;
    check(gfo_match_legal_commands(game,2,0,buffer.data()+1,count-1,&n)==-4&&n==123
        &&std::memcmp(buffer.data(),before.data(),buffer.size()*sizeof(buffer[0]))==0,"atomic short buffer");
    check(gfo_match_legal_commands(game,2,0,buffer.data()+1,count,&n)==0&&n==count,"fill");
    check(std::memcmp(&buffer.front(),&before.front(),sizeof(buffer[0]))==0
        &&std::memcmp(&buffer.back(),&before.back(),sizeof(buffer[0]))==0,"canaries");
    for(size_t i=1;i<=n;++i){uint8_t legal=0;check(gfo_match_legal(game,&buffer[i],&legal)==0&&legal,"all legal");}
    const auto good=buffer;n=123;
    for(auto limit:{size_t{0},size_t{5},size_t(-1)})
        check(gfo_match_legal_commands(game,limit,0,buffer.data(),count,&n)==-2&&n==123,"invalid limit");
    for(auto step:{-1,-2147483647})
        check(gfo_match_legal_commands(game,2,step,buffer.data(),count,&n)==-2&&n==123,"invalid step");
    check(gfo_match_legal_commands(nullptr,2,0,buffer.data(),count,&n)==-1
        &&gfo_match_legal_commands(game,2,0,nullptr,1,&n)==-1
        &&gfo_match_legal_commands(game,2,0,buffer.data(),count,nullptr)==-1,"null inputs");
    check(std::memcmp(buffer.data(),good.data(),buffer.size()*sizeof(buffer[0]))==0,"errors preserve buffer");
    std::vector<gfo_match_command> again(count);
    check(gfo_match_legal_commands(game,2,0,again.data(),count,&n)==0
        && std::memcmp(again.data(),buffer.data()+1,count*sizeof(buffer[0]))==0,"deterministic encoded bytes");
    gfo_match_destroy(game);
    gfo_match_config config{};config.max_decisions=10;config.hand_limit=18;
    config.initial_hp=40;config.initial_mp=10;config.initial_cp=20;config.external_artifacts=1;
    check(gfo_match_create(&config,players,2,1,&game)==0,"external create");
    gfo_match_command pray{};pray.kind=3;gfo_match_status status{};
    check(gfo_match_step(game,&pray,&status)==0&&status.phase==4,"grant fixture");
    n=123;
    check(gfo_match_legal_commands(game,2,0,buffer.data(),count,&n)==-5&&n==123,"grant unsupported");
    check(std::memcmp(buffer.data(),good.data(),buffer.size()*sizeof(buffer[0]))==0,"grant output atomic");
    gfo_match_destroy(game);
    config.external_artifacts=0;config.max_decisions=1;
    check(gfo_match_create(&config,players,2,1,&game)==0
        &&gfo_match_step(game,&pray,&status)==0,"finished fixture");
    check(gfo_match_legal_commands(game,2,0,buffer.data(),count,&n)==-2&&n==123,"finished invalid");
    gfo_match_destroy(game);
    config.max_decisions=10;
    check(gfo_match_create(&config,players,2,1,&game)==0,"confused create");
    gfo_state_player state_players[2]{};state_players[0].hp=40;state_players[1].hp=40;
    state_players[0].confusion_count=1;state_players[1].id=77;
    gfo_state_meta meta{};gfo_state* state=nullptr;
    check(gfo_state_create(&meta,state_players,2,nullptr,0,nullptr,0,&state)==0
        &&gfo_match_load_scene(game,state,0,1,&status)==0,"confused fixture");
    check(gfo_match_legal_commands(game,2,0,buffer.data(),count,&n)==-5&&n==123,"confused unsupported");
    check(std::memcmp(buffer.data(),good.data(),buffer.size()*sizeof(buffer[0]))==0,"rejected outputs atomic");
    gfo_state_destroy(state);gfo_match_destroy(game);
    std::cout<<"C enumeration two-stage/atomic/error checks passed\n";
}
