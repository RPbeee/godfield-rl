#include "godfield/official_state.hpp"
#include <charconv>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace godfield::official;
std::vector<std::string_view> split(std::string_view value,char delimiter) {
    std::vector<std::string_view> result;
    for(std::size_t start=0;;){const auto end=value.find(delimiter,start);result.push_back(value.substr(start,end-start));if(end==value.npos)break;start=end+1;}
    return result;
}
template<class T>T integer(std::string_view value) {
    T number{};const auto result=std::from_chars(value.data(),value.data()+value.size(),number);
    if(result.ec!=std::errc{}||result.ptr!=value.data()+value.size())throw std::runtime_error("bad vector integer");return number;
}
}
int main(int argc,char** argv) {
    if(argc!=2){std::cerr<<"usage: official_active_attack_vectors FILE\n";return 2;}
    std::ifstream input(argv[1]);if(!input){std::cerr<<"cannot open active attack vectors\n";return 2;}
    std::string line;std::size_t count=0,errors=0;
    while(std::getline(input,line)) {
        if(line.empty()||line.front()=='#'||line.starts_with("label\t"))continue;
        const auto f=split(line,'\t');if(f.size()!=14){std::cerr<<"bad vector width\n";return 2;}
        try {
            SnapshotState board;board.is_over=integer<unsigned>(f[1])!=0;
            if(f[12]!="-")for(auto id:split(f[12],',')){PlayerState player;player.id=integer<PlayerId>(id);board.players.push_back(player);}
            ActiveAttackState attack;attack.player_id=integer<PlayerId>(f[2]);attack.target_player_id=integer<PlayerId>(f[3]);attack.guardian=static_cast<Guardian>(integer<unsigned>(f[4]));
            if(f[5]!="-")for(auto id:split(f[5],','))attack.models.push_back(integer<CardId>(id));
            attack.atk=integer<std::int64_t>(f[6]);attack.hit_rate=integer<std::int64_t>(f[7]);attack.mp=integer<std::int64_t>(f[8]);attack.cp=integer<std::int64_t>(f[9]);attack.reversed=integer<unsigned>(f[10])!=0;attack.buying_model=integer<CardId>(f[11]);
            std::string actual;
            try{actual=active_attack_json(project_active_attack(board,std::span{&attack,1}));}
            catch(const std::exception&){actual="ERROR";}
            if(actual!=f[13]){std::cerr<<"active client vector mismatch "<<f[0]<<"\nexpected "<<f[13]<<"\nactual "<<actual<<'\n';return 1;}
            ++count;errors+=actual=="ERROR";
        } catch(const std::exception& error){std::cerr<<"invalid fixture "<<f[0]<<": "<<error.what()<<'\n';return 2;}
    }
    if(!count){std::cerr<<"empty active attack fixture\n";return 2;}
    std::cout<<"Actual client active attack vectors passed: "<<count<<" (expected fresh errors "<<errors<<").\n";
}
