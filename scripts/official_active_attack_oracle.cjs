#!/usr/bin/env node
// Actual pinned attack DTO, fresh caller and display projection; no game executor.
// stdout only. --check compares the separately archived report without writes.
'use strict';
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm');
const {loadPinnedClient,digest}=require('./official_client_oracle.cjs');
let client=null,check=false,vectorsOnly=false;const args=process.argv.slice(2);
for(let i=0;i<args.length;++i){if(args[i]==='--check'&&!check)check=true;else if(args[i]==='--vectors'&&!vectorsOnly)vectorsOnly=true;else if(args[i]==='--client'&&client===null&&i+1<args.length&&!args[i+1].startsWith('--'))client=args[++i];else throw new Error('Usage: node scripts/official_active_attack_oracle.cjs --client PATH [--check] [--vectors]');}
if(client===null)throw new Error('--client is required');
const directory=path.resolve(__dirname,'../data/official-v2');
const {A,B,t,$,box,binding,manifest,originalSource}=loadPinnedClient(['--client',client,'--catalog-dir',directory],'scripts/official_active_attack_oracle.cjs');
if(binding.version!=='official-v2')throw new Error('Expected pinned official-v2');
const facts=JSON.parse(fs.readFileSync(path.join(directory,'cards.json'),'utf8'));
function hash(start,end){const a=originalSource.indexOf(start),b=originalSource.indexOf(end,a);if(a<0||b<a||originalSource.indexOf(start,a+1)>=0)throw new Error('Pinned source helper changed');return digest(originalSource.slice(a,b));}
const helperHashes={attack_decoder:hash('p6(a){var s,r,q,p,o,n,m,l,k,j=a.i(0,"playerId")','\npn(a){'),snapshot_decoder:hash('tn(a){var s,r,q,p,o,n,m=a.i(0,"players"),l=t.j','\num(){'),fresh_caller:hash('fq(a){var s,r,q,p,o,n,m,l,k,j,i,h,g=this,f=a.w','\ncw(a){'),frame_projection:hash('fu(a,b){var s,r,q,p,o,n,m,l,k,j,i,h,g=this','\naa(a){'),summary:hash('iI(a,b,c,d){var s,r,q=this,p=$.r.v(),o=q.d,n=o.length','\nA.fl.prototype={'),target_pane:hash('aT(a){var s=this.f','\nfF(){'),source_pane:hash('fz(a,b,c){var s,r,q=this.c,p=q.i(0,a)','\naz(a,b){'),card_visible_labels:hash('iP(a,a0,a1,a2,a3,a4,a5,a6){var s,r,q,p,o,n,m,l=this','\nA.fj.prototype={')};
const attackDecoder=new vm.Script(`oracle.A.p6(oracle.A.${binding.decodeJSON}(JSON.parse(rawText)))`),snapshotDecoder=new vm.Script(`oracle.A.tn(oracle.A.${binding.decodeJSON}(JSON.parse(rawText)))`);
function decode(raw,snapshot=false){box.rawText=JSON.stringify(raw);return(snapshot?snapshotDecoder:attackDecoder).runInContext(box);}
function decoded(a){return{player_id:a.a,guardian:a.b?.c??null,models:Array.from(a.c),atk:a.d,hit_rate:a.e,mp:a.f,cp:a.r,target_player_id:a.w,reversed:a.x,buying_item_model_id:a.y};}
const side=s=>s===B.a?'a':s===B.b?'b':null;
$.r={v:()=>({P:key=>key,eU:(power,hit_rate)=>({power,hit_rate})})};
$.t=()=>({z:'not-a-roster-user',ax:true,c:{a:{append(){}}}});
const originalCard=A.a4,originalSummary=A.aR;
A.a4=(card,atk,plus,cp,curse,disguised,rate,mp)=>({model:card.a,atk,plus_display:!!plus,cp,curse:curse?.c??null,disguised:!!disguised,hit_rate:rate,mp});
A.aR=(cards,filtered,atk,rate)=>Object.assign(Object.create(A.dA.prototype),{d:cards,e:atk,f:rate,r:filtered});
const labelCache=new Map();
function visibleLabels(widget){const key=JSON.stringify(widget);if(labelCache.has(key))return labelCache.get(key);
    const old={v:A.v,B:A.B,fC:A.fC,a3:A.a3,og:A.og,locale:$.r};const labels=[];
    const node=()=>({style:{},append(){}}),graphic=()=>({a:node(),Y(){},F(){},E(){},a7(){},sD(){},sq(){},a0(){}});
    const templates={atk:'atk({{atk}})',plusAtk:'plus({{atk}})',hitRate:'rate({{hitRate}})',def:'def({{def}})',priceCP:'price({{price}})',costMP:'costMP({{mp}})'};
    const text={P:k=>templates[k]??k,a5:k=>k+'({{hp}};{{damage}};{{mp}};{{cp}};{{curse}};{{value}};{{element}})',bo:k=>'curse('+k+')',hP:k=>'element('+k+')',cJ:(a,b,c)=>c?'plus('+a+')':'attack('+a+','+b+')',eU:(a,b)=>'attack('+a+','+b+')',hJ:a=>'attack('+a+',0)',hK:(a,b)=>b?'plus('+a+')':'attack('+a+',0)'};
    try{A.v=value=>{labels.push(value);return graphic();};A.B=graphic;A.fC=graphic;A.a3=graphic;A.og=graphic;$.r={v:()=>text};
        const object=Object.assign(Object.create(A.dO.prototype),{d:$.p[widget.model-1],a:node(),eL(){},hi(value){labels.push(value);}});
        const curse=widget.curse===null?null:A.jY(widget.curse);
        A.dO.prototype.iP.call(object,null,widget.atk,widget.plus_display,widget.cp,curse,widget.disguised,widget.hit_rate,widget.mp);
    }finally{Object.assign(A,{v:old.v,B:old.B,fC:old.fC,a3:old.a3,og:old.og});$.r=old.locale;}
    labelCache.set(key,labels);return labels;
}
function publicCard(widget){const c=facts[widget.model-1];const out={model:widget.model,atk:null,from_mp:false,hit_rate:null,mp:null,cp:null,curse:null};
    if(c.ability==='atkBy2xMP'){out.atk=widget.atk;out.from_mp=true;}
    else if(c.ability==='attackDyingly'){out.atk=widget.atk>0?widget.atk:c.atk||0;out.hit_rate=widget.atk>0?widget.hit_rate:c.hitRate||0;}
    else if(['counterAtk','counter2xAtk'].includes(c.ability)&&widget.atk!==0)out.atk=widget.atk;
    else if(c.ability==='counterBoost2xMP'&&widget.mp!==0)out.mp=widget.mp;
    else if(c.ability==='counterTakeCP'&&widget.cp!==0)out.cp=widget.cp;
    else if(c.ability==='counterCurse')out.curse=widget.curse;
    return out;
}
function summaryProjection(display){let rendered=null,elementEmitted=null;
    Object.assign(display,{sq(){},hl(element){elementEmitted=element?.c??null;},cC(value){rendered=typeof value==='string'?{kind:value}:{kind:'attack',...value};}});
    const rawIDs=Array.from(display.d,x=>x.a);A.dA.prototype.iI.call(display,null,null,null,null);
    const models=Array.from(display.d,x=>x.a),empty=rawIDs.length===0,isAttack=models.length>0&&display[binding.attackFlag]();
    const first=models.length?display.d[0]:null;
    const power=isAttack?A[binding.attack](display.d,display.e):null;
    const rate=isAttack?(display.f>0?display.f:first.w):null;
    if(empty?rendered?.kind!=='pray':isAttack?rendered?.power!==power||rendered?.hit_rate!==rate:rendered!==null)throw new Error('Actual summary emission differs from projection');
    return{models,prayer:empty,is_attack:isAttack,effective_category:first?(first.y==='categoryWeapons'?'weapons':first.d.c):null,element:models.length?A[binding.attackElement](display.d)?.c??null:null,computed_power:power,effective_rate:rate,rendered,emitted_element:elementEmitted};
}
function frame(){return{Q:false,panes:{a:null,b:null},calls:[],widgets:[],summaries:[],arrow_side:null,guardian_art:null,
    aa(){this.Q=false;this.panes={a:null,b:null};this.calls.push(['clear']);},
    az(s,p){this.panes[side(s)]=p?.c??null;this.calls.push(['pane',side(s),p?.c??null]);},
    u(s){const id=this.panes[side(s)];return this.people?.find(p=>p.c===id)??null;},
    bk(g){this.guardian_art=g?.c??null;this.calls.push(['guardian',this.guardian_art]);},
    af(s,widget){this.calls.push(['widget',side(s),widget]);if(widget)this.widgets.push({side:side(s),...widget});},
    fp(){},aL(s){this.calls.push(['layout',side(s)]);},
    a_(s,display){this.summaries.push({side:side(s),...summaryProjection(display)});},
    b5(s){this.arrow_side=side(s);this.calls.push(['arrow',side(s)]);},
    fu(a,p){A.fs.prototype.fu.call(this,a,p);}
};}
function projectedWidget(w){const labels=visibleLabels(w),card=publicCard(w),ability=facts[w.model-1].ability;
    if(ability==='atkBy2xMP'&&!labels.includes('attack('+card.atk+',0)'))throw new Error('58 visible MP-derived ATK differs');
    if(ability==='attackDyingly'&&!labels.includes('attack('+card.atk+','+card.hit_rate+')'))throw new Error('112 final visible ATK/rate differs');
    if(['counterAtk','counter2xAtk'].includes(ability)){
        const expected=card.atk===null?'counter':'attack('+card.atk+','+(ability==='counterAtk'?(facts[w.model-1].hitRate||0):0)+')';
        if(!labels.includes(expected))throw new Error('Counter visible numeric/generic boundary differs');
    }
    if(ability==='counterBoost2xMP'&&!labels.some(x=>card.mp===null?x==='counter':x.startsWith('boostMP(')&&x.includes(';'+card.mp+';')))throw new Error('MP counter visible/generic boundary differs');
    if(ability==='counterTakeCP'&&!labels.some(x=>card.cp===null?x==='counter':x.startsWith('takeCP(')&&x.includes(';'+card.cp+';')))throw new Error('CP counter visible/generic boundary differs');
    if(ability==='counterCurse'&&!labels.includes('curse('+card.curse+')'))throw new Error('Curse counter visible label differs');
    return{...w,visible_labels:labels,public_card:card};
}
function direct(raw,sourceObject=17){const a=decode(raw),f=frame();A.fs.prototype.fu.call(f,a,sourceObject===null?null:{c:sourceObject});return{decoded:decoded(a),source_object_id:sourceObject,source_side:a.x?'b':'a',guardian_art:f.guardian_art,widgets:f.widgets.map(projectedWidget),summary:f.summaries[0]};}
function fresh(raw){const snapshot=decode(raw,true),people=Array.from(snapshot.a,p=>Object.assign(Object.create(A.c5.prototype),{c:p.a,d:'user'+p.a,w:false,x:p.as??false,hA(){},fq(){}}));
    const roster=new A.h8(A.u(people,t.r),null);const f=frame();f.people=people;
    const app=Object.assign(Object.create(A.fv.prototype),{ay:-1,x:roster,a:{append(){}},e:{a:{},sbh(){},gbw:()=>false},c:{a:{style:{}}},y:{a:{}},f,
        hR(){this.over_seen=true;},fF(){this.activation_seen=true;},over_seen:false,activation_seen:false});
    A.fv.prototype.fq.call(app,snapshot);
    return{decoded_active:snapshot.e?decoded(snapshot.e):null,raw_attack_count:Array.isArray(raw.attacks)?raw.attacks.length:0,is_over:snapshot.f,active_rendered:f.summaries.length>0,over_branch:app.over_seen,command_activation_branch:app.activation_seen,reversed:f.Q,panes:f.panes,arrow_side:f.arrow_side,guardian_art:f.guardian_art,widgets:f.widgets.map(projectedWidget),summaries:f.summaries,calls:f.calls};
}
const decoderCases=[];
for(const raw of [{},{playerId:0,guardian:'earth',itemModelIds:[58],atk:17,hitRate:75,mp:12,cp:9,targetPlayerId:7,reversed:true,buyingItemModelId:235},{playerId:17,targetPlayerId:0},{playerId:999,targetPlayerId:999},{playerId:-1,targetPlayerId:-1,atk:-1,hitRate:-1,mp:-1,cp:-1,buyingItemModelId:-1},{playerId:'17',guardian:'unknown',itemModelIds:[6],atk:'12',hitRate:true,mp:1.5,cp:null,reversed:1,buyingItemModelId:'235'},{itemModelIds:null},{itemModelIds:[6,'7',null,true]},{itemModelIds:[0]},{itemModelIds:[297]}]){
    let output=null,error=false;try{output=decoded(decode(raw));}catch(_){error=true;}decoderCases.push({input:raw,output,error});
}
const cases=[];const run=(label,raw)=>{cases.push({label,input:raw,projection:direct(raw)});};
for(let model=1;model<=296;++model)run('standalone-model',{playerId:17,itemModelIds:[model],targetPlayerId:7});
for(const ids of [[],[207],[207,207],[207,6],[207,207,6],[207,291],[207,293],[207,295],[207,296],[207,58,12],[199,6],[286,210],[285,6],[6,66,83],[6,231,12],[6,...Array(18).fill(12)]])run('ordered-summary-edge',{playerId:17,itemModelIds:ids,targetPlayerId:7});
for(const model of [58,112,181,184,185,186,187,188,210,235,291,292,293])for(const value of [0,17])run('dynamic-widget-and-summary',{playerId:17,guardian:'earth',itemModelIds:[model],atk:value,hitRate:value?73:0,mp:value,cp:value,targetPlayerId:7,reversed:!!value});
for(const ids of [[],[199],[207],[207,207,6],[112],[292]])for(const atk of [-1,9])for(const rate of [-1,0,81])run('summary-override-shape',{playerId:0,itemModelIds:ids,atk,hitRate:rate,targetPlayerId:0});
for(const model of [58,112,181,184,185,186,187,188])for(const value of [-1,0,1])run('visible-card-label-boundary',{playerId:17,itemModelIds:[model],atk:value,hitRate:value,mp:value,cp:value,targetPlayerId:7});
run('112-canonical-static-equivalent',{playerId:17,itemModelIds:[112],atk:0,hitRate:0,targetPlayerId:7});
run('112-canonical-dynamic-equivalent',{playerId:17,itemModelIds:[112],atk:1,hitRate:25,targetPlayerId:7});
const freshCases=[],players=[{id:0,name:'zero'},{id:7,name:'seven'},{id:42,name:'forty-two'}];
function runFresh(label,patch){const input={players,attackTurnPlayerId:7,gf:1,updateCount:1,...patch};let output=null,error=false;try{output=fresh(input);}catch(_){error=true;}freshCases.push({label,input,output,error});}
runFresh('no-active-owner-pane',{attacks:[]});runFresh('missing-attacks-default',{ });runFresh('empty-DTO-is-prayer',{attacks:[{}]});
for(const source of [0,17,999])for(const target of [0,7,42,999])for(const reversed of [false,true])runFresh('source-target-resolution',{attacks:[{playerId:source,itemModelIds:[6],targetPlayerId:target,reversed}]});
for(const target of [0,7,999])for(const buying of [0,235])for(const reversed of [false,true])runFresh('buying-display-gates',{attacks:[{playerId:0,itemModelIds:[5],targetPlayerId:target,buyingItemModelId:buying,reversed}]});
runFresh('tail-invalid-model-is-not-decoded',{attacks:[{playerId:0,itemModelIds:[6],targetPlayerId:7},{playerId:999,itemModelIds:[999999]}]});
runFresh('tail-nonobject-is-not-decoded',{attacks:[{playerId:0,itemModelIds:[6],targetPlayerId:7},'not-an-attack-object']});
runFresh('isOver-skips-active-invalid-model-render',{isOver:true,attacks:[{playerId:0,itemModelIds:[999999],targetPlayerId:7}]});
runFresh('isOver-skips-empty-DTO-render',{isOver:true,attacks:[{}]});
runFresh('isOver-skips-unknown-roster-lookup',{isOver:true,attacks:[{playerId:999,itemModelIds:[6],targetPlayerId:999,buyingItemModelId:235}]});
runFresh('fresh-Fate-one-drop',{attacks:[{playerId:0,itemModelIds:[207,207,6],targetPlayerId:7}]});
runFresh('fresh19-model-column',{attacks:[{playerId:0,itemModelIds:[6,...Array(18).fill(12)],targetPlayerId:7}]});
for(const row of freshCases){const a=row.input.attacks?.[0],ids=players.map(p=>p.id);const expected=!!(a&&!row.input.isOver&&((Number.isInteger(a.playerId)&&!ids.includes(a.playerId))||(a.targetPlayerId>0&&!ids.includes(a.targetPlayerId))));row.expected_client_error=expected;if(row.error!==expected)throw new Error('Unexpected fresh harness result: '+row.label+' '+JSON.stringify(row.input));}
const prayer=cases.find(x=>x.label==='ordered-summary-edge'&&x.input.itemModelIds.length===0);if(!prayer.projection.summary.prayer||prayer.projection.summary.is_attack)throw new Error('Empty prayer distinction changed');
for(const row of freshCases){const {input,output}=row;if(row.error)continue;if(input.isOver&&output.active_rendered)throw new Error('isOver active suppression changed');if(input.attacks?.[0]?.targetPlayerId===0&&output.widgets.some(w=>w.model===235))throw new Error('target0 leaked buying product');}
const static112=cases.find(x=>x.label==='112-canonical-static-equivalent').projection.widgets[0],dynamic112=cases.find(x=>x.label==='112-canonical-dynamic-equivalent').projection.widgets[0];
if(JSON.stringify(static112.visible_labels)!==JSON.stringify(dynamic112.visible_labels)||JSON.stringify(static112.public_card)!==JSON.stringify(dynamic112.public_card))throw new Error('112 equal visible labels must canonicalize to equal cards');
function publicJSON(decodedAttack,projection,over){if(over)return{schema:'official-active-attack-view-v1',attack:null};const s=projection.summary,hasTarget=decodedAttack.target_player_id>0;
    return{schema:'official-active-attack-view-v1',attack:{source:decodedAttack.player_id,guardian:decodedAttack.guardian,models:decodedAttack.models,cards:projection.widgets.map(w=>w.public_card),reversed:decodedAttack.reversed,target:hasTarget?decodedAttack.target_player_id:null,buying_model:hasTarget&&decodedAttack.buying_item_model_id>0?decodedAttack.buying_item_model_id:null,summary:{models:s.models,prayer:s.prayer,is_attack:s.is_attack,element:s.is_attack?s.element:null,power:s.computed_power,hit_rate:s.effective_rate},response:s.models.length?{incoming_category:s.effective_category,incoming_element:s.element,incoming_is_attack:s.is_attack}:null}};
}
const lines=['# official-active-attack-vectors-v48: label over source target guardian models atk rate mp cp reversed buying rosterIDs expectedJSON'];
function vector(label,raw,roster,over=false,expectedError=false){const d=decoded(decode(raw));let expected='ERROR';if(!expectedError){const projection=direct(raw,d.player_id);expected=JSON.stringify(publicJSON(d,projection,over));}lines.push([label,+over,d.player_id,d.target_player_id,d.guardian===null?0:manifest.enums.Guardian.indexOf(d.guardian),d.models.length?d.models.join(','):'-',d.atk,d.hit_rate,d.mp,d.cp,+d.reversed,d.buying_item_model_id,roster.length?roster.join(','):'-',expected].join('\t'));}
for(let i=0;i<cases.length;++i){const row=cases[i],roster=[0,7,17,42];const f=fresh({players:roster.map(id=>({id})),gf:1,updateCount:1,attackTurnPlayerId:7,attacks:[row.input]});if(!f.active_rendered)throw new Error('Known frame was not fresh-rendered');vector('frame'+i+'-'+row.label,row.input,roster);}
for(let i=0;i<freshCases.length;++i){const row=freshCases[i],raw=row.input.attacks?.[0];if(!raw||typeof raw!=='object'||raw.itemModelIds?.some(id=>!Number.isInteger(id)||id<1||id>296))continue;vector('fresh'+i+'-'+row.label,raw,players.map(p=>p.id),!!row.input.isOver,row.error);}
const tsv=lines.join('\n')+'\n';
const directNullSourceCases=[0,999].map(id=>({input:{playerId:id,itemModelIds:[6],targetPlayerId:7},projection:direct({playerId:id,itemModelIds:[6],targetPlayerId:7},null)}));
const counts={direct_null_source_cases:directNullSourceCases.length,decoder_cases:decoderCases.length,standalone_frame_cases:296,ordered_frame_cases:16,dynamic_frame_cases:26,override_frame_cases:36,visible_card_boundary_cases:26,all_frame_cases:cases.length,fresh_caller_cases:freshCases.length,fresh_expected_errors:freshCases.filter(x=>x.error).length,native_tsv_vectors:lines.length-1};
const report={schema_version:1,title:'Actual public active-attack decoder, fresh caller and summary projection v48',source:manifest.client_url,client_sha256:manifest.client_sha256,cards_sha256:manifest.cards_sha256,helper_sha256:helperHashes,methods:['A.p6 and A.tn unchanged real decoders;tn decodes only attacks[0]','A.fs.fu unchanged actual active frame builder with card-widget/summary visual constructors stubbed','A.dA.iI/gf2 unchanged actual summary mutation and attack flag;tz/lc unchanged arithmetic','A.dO.iP unchanged visible-label branch with symbolic translation/visual sinks;public special-card values canonicalize112static-equivalent labels','A.fv.fq unchanged complete fresh caller;fresh-forcing context/spectator and board/DOM display sinks stubbed','A.fv.aT unchanged target-pane branch and A.h8.u/J unchanged roster resolution'],counts,vectors:{path:'data/official-v2/active-attack-vectors-v48.tsv',sha256:digest(tsv),bytes:Buffer.byteLength(tsv),cases:lines.length-1,columns:['label','over','source','target','guardianEnum','models','atk','rate','mp','cp','reversed','buying','rosterIDs','expectedJSON'],boundary:'Known strict-model frame rows plus fresh unknown-roster ERROR cases;malformed decoder/isOver-invalid-model examples remain JSON-only. No-active/null-DTO span has separate caller examples in JSON.'},decoder_cases:decoderCases,frame_cases:cases,direct_null_source_cases:directNullSourceCases,fresh_cases:freshCases,conclusions:['A.tn reads only the first attack;later raw entries are not decoded,rendered or permission to expose a future queue. A supplied isOver suppresses active rendering after board refresh,even when the chosen model would fail if rendered.','Raw empty models draw prayer before gf2;it is not a zero-power attack. A single207 draws neither prayer nor an attack summary. At least two models with leading207 drop exactly one207 before gf2. Repeated leading207 therefore remain meaningful and are not recursively stripped.','gf2 uses positive first-summary ATK,first category weapons,or positive dynamic ATK. Positive summed addon power alone doesnot establish the attack flag. Nonattack summary power/rate are absent even if an arithmetic helper could return a value.','An attack summary uses positive dynamic hitRate when supplied,otherwise the first postdrop model rate. Dynamic ATK is a base override only when positive. CategoryWeapons changes effective reaction category independently of guardian art.','fu supplies different widget overrides for58,112,181/184,187,188 and counterCurse,while keeping the raw column. Models/summary/category/element/display values are separate from physical inventory tokens and authoritative effect resolution.','The fresh caller restores target only for targetPlayerId>0,and restores a positive buyingItemModelId only inside that target branch. A real source player0 resolves from the roster;numeric0 is not an environmental/no-source tag. The actual rosteru lookup throws when source is absent;artificial directfu(null) acceptance is a separate lower-level fact.','The actual fresh caller throws for an unknown positive target before entering the target-pane/buying branch. Target0 bypasses that lookup even when realplayer0 exists. Ending returns before both source/target lookup,so unknown endpoints can remain undrawn there.','The19-model active column renders in this source path. The18-own-hand drawing limit is not applied to an attack model array by this caller.'],limitations:['These are actual received/public-client projection facts,not a legal Match-state validator,actor/phase authority,server recipient selector,damage/death calculator or complete incremental event renderer.','Fresh cases force the full fresh path and use a spectator context. Board player resource/fog/used-hand rendering,ratings,hand widgets,network and final command activation are scaffolding;existing independent state/view oracles cover those board rules.','Decoder coercion and invalid raw examples are recorded separately from a strict typed adapter domain. They do not require accepting every malformed/coerced wire input in a canonical native API.','Artificially supplied null source,actual unknown-roster lookup failure and numeric player0 are separate observations. No source-free authority semantics or permission to act is inferred from this display behavior.','The visual text formatter is a numeric/symbolic sink:actual iI/iP choose emitted branches and numeric arguments;locale typography/DOM construction is not reproduced. Rawwidget argument records are privileged evidence;only cards-only projection and rendered summary belong in the expected public JSON.','No source bundle is embedded or redistributed. No account,network,timer,Firebase or Dart application startup,C++build or game engine execution runs. Existing pins/reports are unchanged.'],network_or_application_startup:false,generator:'node scripts/official_active_attack_oracle.cjs --client /path/main.dart.js [--check] [--vectors]'};
A.a4=originalCard;A.aR=originalSummary;
const output=JSON.stringify(report,null,2)+'\n';if(check&&(fs.readFileSync(path.join(directory,'active-attack-client-v48.json'),'utf8')!==output||fs.readFileSync(path.join(directory,'active-attack-vectors-v48.tsv'),'utf8')!==tsv))throw new Error('Active attack report/vectors differ');process.stdout.write(vectorsOnly?tsv:output);
