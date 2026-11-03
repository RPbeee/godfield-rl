#include "godfield/official_display.hpp"
#include <charconv>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace godfield::official;
std::vector<std::string_view> split(std::string_view value,char delimiter){std::vector<std::string_view> out;for(std::size_t pos=0;;){const auto end=value.find(delimiter,pos);out.push_back(value.substr(pos,end-pos));if(end==value.npos)break;pos=end+1;}return out;}
template<class T>T number(std::string_view value){T out{};const auto r=std::from_chars(value.data(),value.data()+value.size(),out);if(r.ec!=std::errc{}||r.ptr!=value.data()+value.size())throw std::runtime_error("bad fixture number");return out;}
ActiveAttackState frame(std::string_view value){const auto p=split(value,':');if(p.size()!=10)throw std::runtime_error("bad frame width");ActiveAttackState a;
    a.player_id=number<PlayerId>(p[0]);a.guardian=static_cast<Guardian>(number<unsigned>(p[1]));
    if(p[2]!="-")for(auto id:split(p[2],','))a.models.push_back(number<CardId>(id));
    a.atk=number<std::int64_t>(p[3]);a.hit_rate=number<std::int64_t>(p[4]);a.mp=number<std::int64_t>(p[5]);a.cp=number<std::int64_t>(p[6]);a.reversed=number<unsigned>(p[7])!=0;
    a.target_player_id=number<PlayerId>(p[8]);a.buying_model=number<CardId>(p[9]);return a;
}
}
int main(int argc,char** argv){if(argc!=2){std::cerr<<"usage: active_display_vectors FILE\n";return 2;}std::ifstream input(argv[1]);if(!input)return 2;
    std::size_t count=0,errors=0;std::string line;
    while(std::getline(input,line)){if(line.empty()||line[0]=='#'||line.starts_with("label\t"))continue;auto p=split(line,'\t');if(p.size()!=8){std::cerr<<"bad fixture row width\n";return 2;}
        try{SnapshotState board;board.gf=number<std::int64_t>(p[2]);board.is_over=number<unsigned>(p[3])!=0;
            if(p[1]!="-")for(auto id:split(p[1],',')){PlayerState player;player.id=number<PlayerId>(id);board.players.push_back(player);}
            std::vector<ActiveAttackState> initial;if(p[5]!="-")initial.push_back(frame(p[5]));
            std::vector<ActiveDisplayEvent> events;
            if(p[6]!="-")for(auto encoded:split(p[6],'|')){
                if(encoded.starts_with("N:"))events.emplace_back(NextAttack{frame(encoded.substr(2))});
                else if(encoded.starts_with("T:"))events.emplace_back(SetTargetPlayer{number<PlayerId>(encoded.substr(2))});
                else if(encoded.starts_with("G:"))events.emplace_back(AdvanceGF{number<PlayerId>(encoded.substr(2))});
                else throw std::runtime_error("unknown fixture event");
            }
            std::string actual;
            try{ActiveDisplay display(board,initial,number<PlayerId>(p[4]));display.apply(events);actual=active_display_json(display.view());}
            catch(const std::exception&){actual="ERROR";}
            if(actual!=p[7]){std::cerr<<"active display mismatch "<<p[0]<<"\nexpected "<<p[7]<<"\nactual "<<actual<<'\n';return 1;}++count;errors+=actual=="ERROR";
        }catch(const std::exception& e){std::cerr<<"invalid fixture "<<p[0]<<": "<<e.what()<<'\n';return 2;}
    }
    if(!count)return 2;std::cout<<"Actual client active display vectors passed: "<<count<<" (expected lookup/errors "<<errors<<").\n";
}
