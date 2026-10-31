#include "godfield/official_state.hpp"
#include <array>
#include <charconv>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace godfield::official;
std::vector<std::string_view> split(std::string_view s,char separator){std::vector<std::string_view> out;std::size_t start=0;for(;;){auto at=s.find(separator,start);out.push_back(s.substr(start,at==s.npos?at:at-start));if(at==s.npos)break;start=at+1;}return out;}
std::int64_t number(std::string_view s){std::int64_t n=0;auto r=std::from_chars(s.data(),s.data()+s.size(),n);if(r.ec!=std::errc{}||r.ptr!=s.data()+s.size())throw std::runtime_error("malformed integer");return n;}
std::vector<std::int64_t> numbers(std::string_view s){std::vector<std::int64_t> out;if(s=="-")return out;for(auto v:split(s,','))out.push_back(number(v));return out;}
std::vector<PlayerId> player_ids(std::string_view s){std::vector<PlayerId> out;for(auto n:numbers(s))out.push_back(static_cast<PlayerId>(n));return out;}
void require(bool yes,const char* message){if(!yes)throw std::runtime_error(message);}
SnapshotState state(std::string_view teams,std::int64_t dead){SnapshotState s;const auto t=numbers(teams);for(std::size_t i=0;i<t.size();++i){PlayerState p;p.id=i;p.team=static_cast<Team>(t[i]);p.dead=dead&(std::int64_t{1}<<i);s.players.push_back(p);}return s;}
PlayerState own(){PlayerState p;p.id=0;return p;}
}
int main(int argc,char** argv){
    if(argc!=2){std::cerr<<"usage: official_state_vectors_test state-vectors.tsv\n";return 2;}
    std::ifstream input(argv[1]);if(!input){std::cerr<<"cannot open fresh-state vectors\n";return 2;}
    std::string line;std::size_t lineno=0;std::array<std::size_t,5> counts{};
    while(std::getline(input,line)){
        ++lineno;if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty()||line.front()=='#')continue;
        try{
            const auto f=split(line,'\t');
            if(f[0]=="P"){
                require(f.size()==7,"relation field count");const auto s=state(f[1],number(f[2]));
                require(alive_players(s)==player_ids(f[3]),"actual h7.aG alive mismatch");require(ending_display_candidates(s)==player_ids(f[4]),"actual h7.lV display group mismatch");
                std::string matrix;for(const auto& p:s.players)for(const auto& q:s.players)matrix+=is_enemy(p,q)?'1':'0';if(matrix.empty())matrix="-";
                require(matrix==f[5],"actual c5.ce enemy matrix mismatch");
                if(!s.players.empty()){const auto enemies=split(f[6],';');require(enemies.size()==s.players.size(),"enemy row count");for(std::size_t i=0;i<s.players.size();++i)require(enemy_players(s,i)==player_ids(enemies[i]),"actual h7.dR enemy subset mismatch");}
                ++counts[0];
            }else if(f[0]=="F"){
                require(f.size()==7,"fog field count");auto s=state(f[1],number(f[2]));const auto viewer=number(f[3]);s.is_over=number(f[5]);
                std::optional<PlayerId> id;if(viewer>=0){id=viewer;if(number(f[4]))s.players.at(viewer).curses={Curse::fog};}
                const auto v=project_view(s,id);std::vector<PlayerId> hidden;for(const auto& p:v.players){if(p.fogged)hidden.push_back(p.id);require(p.fogged==!p.status.has_value(),"fresh fog status presence disagrees with overlay");}
                require(hidden==player_ids(f[6]),"extracted actual fresh snapshot fog predicate mismatch");++counts[1];
            }else if(f[0]=="G"){
                require(f.size()==8,"visible model field count");SnapshotState s;s.players={own()};s.players[0].dead=number(f[3]);s.is_over=number(f[4]);
                ItemInstance item;item.token=0;item.model=number(f[1]);if(number(f[2]))item.fake_model=number(f[2]);item.used=number(f[5]);s.players[0].items={item};
                const auto v=project_view(s,0);require(v.hand.size()==1&&v.hand[0].face==number(f[6])&&v.hand[0].reference==number(f[7])
                    &&v.hand[0].used==item.used&&v.hand[0].disguised==item.fake_model.has_value(),"actual aC.gL/l_.$0 visible face or reference mismatch");++counts[2];
            }else if(f[0]=="C"){
                require(f.size()==6,"curse field count");SnapshotState s;s.players={own()};for(auto c:numbers(f[1]))s.players[0].curses.push_back(static_cast<Curse>(c));
                s.players[0].dead=number(f[2]);s.players[0].confusion_count=number(f[3]);const auto v=project_view(s,0);std::vector<std::int64_t> visible;
                for(auto c:v.players[0].status->curses)visible.push_back(static_cast<int>(c));require(visible==numbers(f[4])&&v.players[0].status->confused==bool(number(f[5])),"actual tL Set→dA/hP fresh curse/confusion mismatch");++counts[3];
            }else if(f[0]=="H"){
                require(f.size()==3,"hand sort field count");SnapshotState s;s.players={own()};if(f[1]!="-")for(auto descriptor:split(f[1],',')){
                    const auto d=split(descriptor,':');require(d.size()==3,"item descriptor field count");ItemInstance item;item.token=s.players[0].items.size();item.model=number(d[0]);if(number(d[1]))item.fake_model=number(d[1]);item.used=number(d[2]);s.players[0].items.push_back(item);
                }
                const auto v=project_view(s,0);std::vector<std::int64_t> order;for(const auto& item:v.hand)order.push_back(item.token);require(order==numbers(f[2]),"actual fG.hs(false) fresh hand order mismatch");++counts[4];
            }else throw std::runtime_error("unknown fresh-state vector kind");
        }catch(const std::exception& e){std::cerr<<argv[1]<<':'<<lineno<<": "<<e.what()<<"\nvector: "<<line<<'\n';return 1;}
    }
    if(!input.eof()||counts!=std::array<std::size_t,5>{7163,5184,7104,2080,609}){std::cerr<<"incomplete fresh-state vectors: ";for(auto n:counts)std::cerr<<n<<' ';std::cerr<<'\n';return 1;}
    std::cout<<"Official fresh-state vectors passed (22140 actual-client/helper/branch cases, not server transitions).\n";
}
