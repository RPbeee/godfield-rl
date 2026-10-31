#!/usr/bin/env node
// Offline actual-client fresh-state helpers; no application startup or network.
'use strict';
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),crypto=require('node:crypto');
const directory=path.resolve(__dirname,'../data/official-v1');
const manifest=JSON.parse(fs.readFileSync(path.join(directory,'manifest.json'),'utf8'));
const args=process.argv.slice(2);if(args.length!==2||args[0]!=='--client')throw new Error('Usage: --client /path/to/main.dart.js');
const digest=b=>crypto.createHash('sha256').update(b).digest('hex');
const bytes=fs.readFileSync(args[1]),cards=fs.readFileSync(path.join(directory,'cards.json'));
if(digest(bytes)!==manifest.client_sha256||digest(cards)!==manifest.cards_sha256)throw new Error('Pinned input mismatch');
let source=bytes.toString('utf8');const marker='var s=A.wG';if(source.split(marker).length!==2)throw new Error('Export marker changed');
const fogOuter='l.ay.ak(B.q)&&!l.x&&!p',fogInner='i===B.p&&m!==l||i!==j';
for(const predicate of [fogOuter,fogInner])if(source.split(predicate).length!==2)throw new Error('Fresh fog branch not unique');
source=source.replace(marker,'globalThis.oracle={A,B,J,t,$};var s=A.wG');
const box={dartMainRunner(){}};vm.createContext(box);vm.runInContext(source,box,{timeout:5000});box.cards=cards.toString('utf8');
vm.runInContext('oracle.A.ty(oracle.A.nF(JSON.parse(cards)))',box,{timeout:5000});
const {A,B,t,$}=box.oracle;if($.p.length!==296)throw new Error('Catalog loader changed');
const teams=[B.p,B.ad,B.ae,B.af,B.ag];const names=['','cold','fever','hell','heaven','fog','dream','flash','darkcloud'];
const record=id=>$.p[id-1],enc=a=>a.length?a.join(','):'-';
const players=(team,dead)=>team.map((team,i)=>Object.assign(Object.create(A.c5.prototype),{c:i,d:'p'+i,r:teams[team],x:!!(dead&(1<<i))}));
const roster=p=>new A.h7(A.u(p,t.r),null);
const freshFog=vm.runInContext('(function(l,m,p){const B=oracle.B,i=m.r,j=l.r;return ('+fogOuter+')&&('+fogInner+');})',box);
const lines=['# official-state-vectors-v1: P actual relations/lists; F extracted fresh-fog branch; G actual face/tooltip; C actual wireSet+dA; H actual fresh sort'];
const counts={P:0,F:0,G:0,C:0,H:0};let seed=0x6d2b79f5;
const random=n=>{seed^=seed<<13;seed^=seed>>>17;seed^=seed<<5;return(seed>>>0)%n;};
function relations(team,dead){const p=players(team,dead),r=roster(p);lines.push(['P',enc(team),dead,enc(r.aG().map(x=>x.c)),enc(r.lV().map(x=>x.c)),p.map(a=>p.map(b=>+a.ce(b)).join('')).join('')||'-',p.map(a=>enc(r.dR(a).map(x=>x.c))).join(';')||'-'].join('\t'));++counts.P;}
for(let n=0;n<=9;++n)for(let layout=0;layout<5;++layout)for(let mask=0;mask<(1<<n);++mask)relations(Array.from({length:n},(_,i)=>(i+layout)%5),mask);
for(let k=0;k<2048;++k){const n=random(10);relations(Array.from({length:n},()=>random(5)),random(1<<n));}
for(let n=1;n<=9;++n)for(let k=0;k<24;++k){const team=Array.from({length:n},()=>random(5)),dead=random(1<<n),p=players(team,dead),r=roster(p);
 for(let viewer=-1;viewer<n;++viewer)for(const fog of [0,1])for(const over of [0,1]){const own=p[viewer];if(own)own.ay={ak:c=>fog&&c===B.q};const hidden=own?r.aG().filter(target=>freshFog(own,target,!!over)).map(x=>x.c):[];
 lines.push(['F',enc(team),dead,viewer,fog,over,enc(hidden)].join('\t'));++counts.F;}}
// Tooltip l_.$0 executes unchanged; only its visual widget builder is replaced.
A.a4=card=>card;
function model(id,fake,dead,over,used){const a=Object.assign(Object.create(A.aC.prototype),{e:record(id),f:fake?record(fake):null,w:used?{}:null});let reference;
 const controller={x:{J:()=>({x:!!dead})},f:{as:!!over},ct:card=>{reference=card.a;}};
 $.t=()=>({z:'offline-viewer',w:Object.assign(Object.create(A.H.prototype),{ch:controller})});
 new A.l_(a).$0();lines.push(['G',id,fake,dead,over,used,a.gL().a,reference].join('\t'));++counts.G;}
for(let id=1;id<=296;++id)for(const fake of [0,id,id%296+1])for(const dead of [0,1])for(const over of [0,1])for(const used of [0,1])model(id,fake,dead,over,used);
// Fresh decoder Set, disease normalization and death flag remain real client code.
// Only curse widget construction/removal and status visuals are no-op stubs.
A.ur=()=>({a:{},l(){}});
const decode=new vm.Script('oracle.A.tL(oracle.A.nF(JSON.parse(cursesText)))');
function curses(input,dead,confusion){box.cursesText=JSON.stringify({curses:input});const d=decode.runInContext(box);
 const p=Object.assign(Object.create(A.c5.prototype),{ay:A.aq(t.E,t.z),z:{a:{append(){}}},cx:confusion,sc8(value){this.cx=value;},Q:{fU(){}},as:{sS(){}},at:{sS(){}},ax:{sS(){}},db:{c:{fU(){}}},hB(){}});
 const it=A.mU(d.y,d.y.r,A.y(d.y).c);while(it.B())p.dA(it.d);if(dead)p.hP();
 const m=p.ay,it2=new A.bH(m,m.r,m.e,A.y(m).h('bH<1>')),out=[];while(it2.B())out.push(names.indexOf(it2.d.c));out.sort((a,b)=>a-b);
 lines.push(['C',enc(input.map(n=>names.indexOf(n))),dead,confusion,enc(out),+(p.cx>0)].join('\t'));++counts.C;}
for(const input of [[],['cold','fog','cold'],['cold','fog','fever','cold'],['fog','dream','flash','darkcloud','heaven','cold']])for(const dead of [0,1])for(const c of [-1,0,1,9])curses(input,dead,c);
for(let k=0;k<2048;++k)curses(Array.from({length:random(13)},()=>names[random(8)+1]),random(2),random(12)-2);
function hand(input){const cards=input.map(([id,fake,used],i)=>Object.assign(Object.create(A.aC.prototype),{e:record(id),f:fake?record(fake):null,w:used?{}:null,c:i,at:0}));const inventory={c:A.u(cards,t.k)};
 A.fG.prototype.hs.call(inventory,false);lines.push(['H',input.length?input.map(a=>a.join(':')).join(','):'-',enc(inventory.c.map(a=>a.c))].join('\t'));++counts.H;}
for(let n=0;n<=18;++n)for(let k=0;k<32;++k)hand(Array.from({length:n},()=>[random(296)+1,random(2)?random(296)+1:0,random(2)]));
hand([[235,6,0],[6,235,0],[235,0,1],[235,0,0],[6,235,0]]);
const output=lines.join('\n')+'\n';fs.writeFileSync(path.join(directory,'state-vectors.tsv'),output);
const metadata={schema_version:1,source:manifest.client_url,client_sha256:manifest.client_sha256,cards_sha256:manifest.cards_sha256,vectors_sha256:digest(output),counts,
 purpose:'Fresh client relations/display/visible instances only; not server transitions or a winner decision.',network_or_application_startup:false,
 methods:['c5.ce','h7.aG/dR/lV','aC.gL','l_.$0 (tooltip widget builder stubbed)','tL/pz + c5.dA/hP (visuals stubbed)','fG.hs(false)'],
 extracted_predicates:{fog_outer:fogOuter,fog_enemy:fogInner},generator:'node scripts/official_state_oracle.cjs --client /path/to/main.dart.js'};
fs.writeFileSync(path.join(directory,'state-vectors.manifest.json'),JSON.stringify(metadata,null,2)+'\n');console.log(JSON.stringify(metadata));
