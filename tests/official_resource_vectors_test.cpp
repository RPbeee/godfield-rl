#include "godfield/official_match.hpp"
#include <array>
#include <charconv>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
namespace {
std::vector<std::string_view> split(std::string_view s){std::vector<std::string_view> out;std::size_t start=0;for(;;){auto end=s.find('\t',start);out.push_back(s.substr(start,end==s.npos?end:end-start));if(end==s.npos)break;start=end+1;}return out;}
std::int64_t integer(std::string_view s){std::int64_t n;auto r=std::from_chars(s.data(),s.data()+s.size(),n);if(r.ec!=std::errc{}||r.ptr!=s.data()+s.size())throw std::runtime_error("bad integer");return n;}
}
int main(int argc,char** argv){using namespace godfield::official;if(argc!=2){std::cerr<<"usage: official_resource_vectors_test fixture.tsv\n";return 2;}std::ifstream f(argv[1]);if(!f){std::cerr<<"cannot open resource vectors\n";return 2;}
 std::string line;std::size_t lineno=0,count=0;while(std::getline(f,line)){++lineno;if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty()||line[0]=='#')continue;
  try{const auto v=split(line);if(v.size()!=9||v[0]!="R")throw std::runtime_error("bad row shape");PlayerState p;p.hp=integer(v[1]);p.mp=integer(v[2]);p.cp=integer(v[3]);p.name="unchanged";const auto resource=integer(v[4]);if(resource<0||resource>2)throw std::runtime_error("resource range");
   change_resource(p,static_cast<Resource>(resource),integer(v[5]));if(p.hp!=integer(v[6])||p.mp!=integer(v[7])||p.cp!=integer(v[8])||p.name!="unchanged"||p.dead)throw std::runtime_error("actualclient resource mismatch: native="+std::to_string(p.hp)+","+std::to_string(p.mp)+","+std::to_string(p.cp));++count;
  }catch(const std::exception& e){std::cerr<<argv[1]<<':'<<lineno<<": "<<e.what()<<"\nvector: "<<line<<'\n';return 1;}}
 if(!f.eof()||count!=7898){std::cerr<<"incomplete resource vectors: "<<count<<'\n';return 1;}std::cout<<"Official resource vectors passed (7898 actual c5 event updates; scheduling not asserted).\n";
}
