#include "godfield/official_rules.hpp"
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
std::vector<std::string_view> split(std::string_view text,char separator){std::vector<std::string_view> out;std::size_t start=0;for(;;){auto end=text.find(separator,start);out.push_back(text.substr(start,end==text.npos?end:end-start));if(end==text.npos)break;start=end+1;}return out;}
std::int64_t number(std::string_view text){std::int64_t value=0;auto r=std::from_chars(text.data(),text.data()+text.size(),value);if(r.ec!=std::errc{}||r.ptr!=text.data()+text.size())throw std::runtime_error("invalid fixture integer");return value;}
Attribute attribute(std::string_view text){auto v=number(text);if(v==-1)return {};if(v<0||v>5)throw std::runtime_error("invalid fixture attribute");return static_cast<Element>(v);}
int attr_number(Attribute a){return a?static_cast<int>(*a):-1;}
std::vector<CardId> composition(std::string_view text){std::vector<CardId> out;if(text=="-")return out;for(auto s:split(text,',')){auto n=number(s);if(n<1||n>catalog_count)throw std::runtime_error("fixture model outside official catalog");out.push_back(static_cast<CardId>(n));}return out;}
VisibleSelection one(std::string_view text){auto p=split(text,':');if(p.size()!=2)throw std::runtime_error("malformed visible model");auto id=number(p[0]),used=number(p[1]);if(id<1||id>catalog_count||used<0||used>1)throw std::runtime_error("visible model range");return {static_cast<CardId>(id),used!=0};}
std::vector<VisibleSelection> selections(std::string_view text){std::vector<VisibleSelection> out;if(text=="-")return out;for(auto s:split(text,','))out.push_back(one(s));return out;}
void equal(std::int64_t actual,std::int64_t expected,const char* field){if(actual!=expected)throw std::runtime_error(std::string(field)+" actual="+std::to_string(actual)+" client="+std::to_string(expected));}
}
int main(int argc,char** argv){
    if(argc!=2){std::cerr<<"usage: official_client_vectors_test fixture.tsv\n";return 2;}
    std::ifstream input(argv[1]);if(!input){std::cerr<<"cannot read official client vectors: "<<argv[1]<<'\n';return 2;}
    std::string line;std::size_t lineno=0;std::array<std::size_t,4> counts{};
    while(std::getline(input,line)){
        ++lineno;if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty()||line[0]=='#')continue;
        try{
            const auto v=split(line,'\t');
            if(v[0]=="C"){
                if(v.size()!=7)throw std::runtime_error("composition field count");const auto ids=composition(v[2]);
                equal(attack_power(ids,number(v[1])),number(v[3]),"attack_power");equal(attr_number(attack_element(ids)),number(v[4]),"attack_element");
                equal(attr_number(defense_element(ids)),number(v[5]),"defense_element");equal(magic_cost(ids),number(v[6]),"magic_cost");++counts[0];
            }else if(v[0]=="R"){
                if(v.size()!=6)throw std::runtime_error("reaction field count");const auto id=static_cast<CardId>(number(v[1]));const auto category=static_cast<Category>(number(v[2]));const auto a=attribute(v[3]);
                equal(static_cast<int>(reaction_for(id,category,a)),number(v[4]),"reaction");equal(attribute_defense_allowed(id,a),number(v[5]),"attribute_defense");++counts[1];
            }else if(v[0]=="M"){
                if(v.size()!=4)throw std::runtime_error("main append field count");const auto s=selections(v[1]);equal(can_append_main(s,one(v[2])),number(v[3]),"main_append");++counts[2];
            }else if(v[0]=="D"){
                if(v.size()!=8)throw std::runtime_error("response append field count");const auto s=selections(v[5]);
                const ResponseContext c{number(v[1])!=0,static_cast<Category>(number(v[2])),attribute(v[3]),number(v[4])!=0};
                equal(can_append_response(s,one(v[6]),c),number(v[7]),"response_append");++counts[3];
            }else throw std::runtime_error("unknown vector kind");
        }catch(const std::exception& e){std::cerr<<argv[1]<<':'<<lineno<<": "<<e.what()<<"\nvector: "<<line<<'\n';return 1;}
    }
    if(!input.eof()||counts!=std::array<std::size_t,4>{4878,16576,12336,16480}){std::cerr<<"incomplete client vectors: "<<counts[0]<<'/'<<counts[1]<<'/'<<counts[2]<<'/'<<counts[3]<<'\n';return 1;}
    std::cout<<"Official public-client vectors passed (50270 pure-helper cases; server transitions not asserted).\n";
}
