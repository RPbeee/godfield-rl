#include "godfield/official_match.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
using namespace godfield::official;
void compare(const Match& m) {
    const auto id=*m.snapshot().actor;auto v=m.view(id);
    // Align own slot order with the executable Match, as gfrl does.
    auto sorted=v.board.hand;v.board.hand.clear();
    for(auto i:m.snapshot().state.players[id==1?0:1].items)
        v.board.hand.push_back(*std::find_if(sorted.begin(),sorted.end(),[&](auto h){return h.token==i.token;}));
    PublicSelection p(v);
    if(p.legal_kinds()!=m.legal_kinds())std::abort();
    for(auto k:m.legal_kinds()) {
        std::vector<ItemToken> prefix;
        const auto visit=[&](auto&& visit)->void {
            auto a=m.legal_extensions(k,prefix,{true}),b=p.legal_extensions(k,prefix,{true});
            if(a!=b)std::abort();
            if(prefix.size()==3)return;
            for(auto token:a.next) {prefix.push_back(token);visit(visit);prefix.pop_back();}
        };visit(visit);
    }
    // Unused foreign truth has no representation in PublicSelection.
    for(const auto& player:p.snapshot().state.players)if(player.id!=id)
        for(const auto& item:player.items)if(!item.used)std::abort();
}
int main() {
    SnapshotState s;s.gf=1;
    PlayerState a;a.id=1;a.curses={Curse::dream};a.items={{1,19,{},false},{2,135,{},false},{3,195,195,false},{4,3,{},false}};
    PlayerState b;b.id=2;b.items={{10,19,{},false},{11,135,{},false},{12,5,{},false}};
    s.players={a,b};MatchConfig c;c.initial_hand=0;c.external_artifacts=true;
    Match m(c);m.load_scene(s,2,0);compare(m);
    m.step({2,CommandKind::play,{10},1});compare(m);
    m.step({1,CommandKind::defend,{2},{}});
    // BUY leads to a public quote; both selection phases use the same tree.
    m.load_scene(s,2,0);m.step({2,CommandKind::buy,{12},1});compare(m);
    PurchaseResponseAction r{{1,CommandKind::defend,{},{}},ItemToken{1}};
    m.purchase_response_step(r);compare(m);
    std::cout<<"public selection: main,response,purchase and all canonical prefixes through depth3 agree\n";
}
