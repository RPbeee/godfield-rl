#!/usr/bin/env node
// Actual incremental active-display branches, separate from server/game transitions.
// stdout JSON by default; --vectors emits TSV; --check compares both archives.
'use strict';
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm');
const {digest}=require('./official_client_oracle.cjs');
let client=null,check=false,vectorsOnly=false;const args=process.argv.slice(2);
for(let i=0;i<args.length;++i){if(args[i]==='--check'&&!check)check=true;else if(args[i]==='--vectors'&&!vectorsOnly)vectorsOnly=true;else if(args[i]==='--client'&&client===null&&i+1<args.length&&!args[i+1].startsWith('--'))client=args[++i];else throw new Error('Usage: node scripts/official_active_display_oracle.cjs --client PATH [--check] [--vectors]');}
if(client===null)throw new Error('--client is required');
const directory=path.resolve(__dirname,'../data/official-v2');
// Reuse only the frozen v48 harness factory, before its cases/output generation.
// This still executes the actual pinned functions in a new isolated client VM.
const helperPath=path.join(__dirname,'official_active_attack_oracle.cjs'),helperBytes=fs.readFileSync(helperPath);
const helperSHA='118f2e4414adde5062eef22be62228eaf656aa53cdb2ef736be430febfbbdc65';
if(digest(helperBytes)!==helperSHA)throw new Error('Frozen active-attack helper changed');
const helperText=helperBytes.toString('utf8'),cut=helperText.indexOf('const decoderCases=[];');
if(cut<0)throw new Error('Frozen harness boundary absent');
const prefix=helperText.slice(0,cut).replace(/^#![^\n]*\n/,'');
const makeHarness=new Function('require','__dirname','process',prefix+'\nreturn {A,B,t,$,box,binding,manifest,originalSource,decode,decoded,summaryProjection,publicCard};');
const {A,B,t,$,box,binding,manifest,originalSource,decode,decoded,summaryProjection,publicCard}=makeHarness(require,__dirname,{argv:['node','v48-helper','--client',client]});
function hash(start,end){const a=originalSource.indexOf(start),b=originalSource.indexOf(end,a);if(a<0||b<a||originalSource.indexOf(start,a+1)>=0)throw new Error('Pinned source branch changed');return digest(originalSource.slice(a,b));}
const helperHashes={event_decoder:hash('to(a){var s,r,q,p,o,n,m,l,k,j,i,h,g,f,e,d,c,b=a.i(0,"action")','\nc1:function'),next_attack:hash('case 82:s="nextAttack"===o?97:98','case 98:s="setTargetPlayer"===o?100:101'),set_target:hash('case 98:s="setTargetPlayer"===o?100:101','case 101:s="hit"===o?117:118'),advance_gf:hash('case 14:s="advanceGF"===o?17:18','case 18:s="attackByGuardian"===o?36:37'),clear_frame:hash('aa(a){var s,r,q=this,p=null,o=q.r','\nA.kc.prototype={'),target_pane:hash('aT(a){var s=this.f','\nfF(){'),fresh_caller:hash('fq(a){var s,r,q,p,o,n,m,l,k,j,i,h,g=this,f=a.w','\ncw(a){')};
const eventDecoder=new vm.Script(`oracle.A.to(oracle.A.${binding.decodeJSON}(JSON.parse(rawEventText)))`);
function decodeEvent(raw){box.rawEventText=JSON.stringify(raw);return eventDecoder.runInContext(box);}
const side=s=>s===B.a?'left':s===B.b?'right':null;
const node=()=>({style:{},append(){}});
// Synchronous Future shell: control flow remains the complete actual ks.k branch.
A.l=()=>({});A.m=fn=>fn;A.k=fn=>fn(0,null);A.b=(value,fn)=>fn(0,value);A.j=value=>value;A.i=value=>{throw value;};A.o=()=>{};
function makeWorld(input){let active=null,buying=null,activeCards=[],insideFU=false,current=null,coreComplete=false;
    const people=input.roster.map(id=>Object.assign(Object.create(A.c5.prototype),{c:id,d:'user'+id,r:B.p,x:false,w:false,cx:0,CW:null,ay:{ak:()=>false},fq(){},hA(){}}));
    const roster=new A.h8(A.u(people,t.r),null),f={Q:false,r:null,w:null,z:null,y:null,a:node(),d:A.u([],t.c),e:A.u([],t.c),panes:{left:null,right:null},selected:null,summaries:{left:null,right:null},calls:[],
        aa(){A.fs.prototype.aa.call(this,0);active=null;buying=null;activeCards=[];},
        az(s,p){this.panes[side(s)]=p?.c??null;this.calls.push(['pane',side(s),p?.c??null]);if(current==='advanceGF'&&p!==null)coreComplete=true;},
        u(s){const id=this.panes[side(s)];return id===null?null:people.find(p=>p.c===id)??null;},
        bk(g){this.y=g;},bg(){this.z=null;},b5(s){this.selected=side(s);this.calls.push(['select',side(s)]);},
        a6(s){return A.fs.prototype.a6.call(this,s);},aO(s){return A.fs.prototype.aO.call(this,s);},
        au(s){this.a6(s).length=0;},
        af(s,widget){this.a6(s).push({c:widget===null?null:{d:$.p[widget.model-1]},d:null,aX(){},l(){}});if(widget){if(insideFU)activeCards.push(publicCard(widget));else if(active)buying=widget.model;}},
        fp(){},aL(){},a_(s,summary){this.summaries[side(s)]=summary?summaryProjection(summary):null;},
        fu(dto,p){insideFU=true;try{A.fs.prototype.fu.call(this,dto,p);}finally{insideFU=false;}active=decoded(dto);if(current==='nextAttack')coreComplete=true;}
    };
    const gf={e:0,d:null,a:node(),sbh(value){this.e=value;},gbw:()=>false};
    const app=Object.assign(Object.create(A.fv.prototype),{ay:-1,x:roster,a:node(),e:gf,c:{a:node()},y:{a:node()},f,w:{ep(){}},ch:false,
        A(){return null;},fF(){},hR(){this.ch=true;},cw(){},ix(){},r:{bI(){},dI(){},e3(){}},
        aT(p){A.fv.prototype.aT.call(this,p);if(current==='setTargetPlayer')coreComplete=true;}
    });
    const controller=Object.assign(Object.create(A.ks.prototype),{a:app});
    $.t=()=>({z:'not-a-roster-user',ax:true,c:{a:node()}});
    const raw={players:input.roster.map(id=>({id})),gf:input.gf,updateCount:1,isOver:input.over,attackTurnPlayerId:input.turn,attacks:input.initial===null?[]:[input.initial]};
    let initError=false;try{A.fv.prototype.fq.call(app,decode(raw,true));}catch(_){initError=true;}
    const view=()=>{let attack=null;if(active){const summary=f.summaries[f.Q?'right':'left'];if(!summary)throw new Error('Active frame lost its summary');attack={source:active.player_id,guardian:active.guardian,models:active.models,cards:activeCards,reversed:f.Q,target:f.selected===null?null:f.panes[f.selected],buying_model:buying,summary:{models:summary.models,prayer:summary.prayer,is_attack:summary.is_attack,element:summary.is_attack?summary.element:null,power:summary.computed_power,hit_rate:summary.effective_rate},response:summary.models.length?{incoming_category:summary.effective_category,incoming_element:summary.element,incoming_is_attack:summary.is_attack}:null};}
        return{schema:'official-active-display-view-v1',gf:gf.e,reversed:f.Q,panes:{left:f.panes.left,right:f.panes.right},selected_side:f.selected,attack};};
    return{initError,view,apply(event){current=event.action;coreComplete=false;let error=false;try{A.ks.prototype.k.call(controller,decodeEvent(event));}catch(_){error=true;}return{full_handler_error:error,core_error:error&&!coreComplete,core_complete:coreComplete,client_partial_view:view()};}};
}
const N=attack=>({action:'nextAttack',attack}),T=id=>({action:'setTargetPlayer',playerId:id}),G=id=>({action:'advanceGF',playerId:id});
const base=(source=0,reversed=false,target=7,buying=0,models=[6])=>({playerId:source,itemModelIds:models,reversed,targetPlayerId:target,buyingItemModelId:buying});
const cases=[];
function run(label,initial,events=[],patch={}){const input={roster:[0,7,42],gf:17,over:false,turn:7,initial,events,...patch},world=makeWorld(input),initialView=world.view(),steps=[];let coreError=world.initError,fullError=world.initError;
    if(!coreError)for(const event of events){const result=world.apply(event);steps.push({event,...result});fullError||=result.full_handler_error;coreError||=result.core_error;if(result.full_handler_error)break;}
    const adapterError=coreError||input.gf>Number.MAX_SAFE_INTEGER-events.filter(e=>e.action==='advanceGF').length||!!input.adapter_strict_error;
    const output=world.view();cases.push({label,input,initial_client_error:world.initError,initial_view:initialView,steps,full_client_error:fullError,core_error:coreError,adapter_expected_error:adapterError,client_final_or_partial_view:output,expected_native_json:adapterError?'ERROR':JSON.stringify(output)});
}
run('fresh-noattack-turn7',null);run('fresh-noattack-turn0',null,[],{turn:0});run('fresh-noattack-unknown-turn-error',null,[],{turn:999});
run('fresh-ending-noattack-no-turn-lookup',null,[],{over:true,turn:999,roster:[]});
for(const reversed of [false,true]){
    const initial=base(0,reversed,7,235);
    run('fresh-target-buy-'+reversed,initial);
    run('same-source-highlights-keeps-opposite-and-buy-'+reversed,initial,[T(0)]);
    run('retarget-keeps-existing-buy-'+reversed,initial,[T(42)]);
    run('retarget-then-source-keeps-opposite-'+reversed,initial,[T(42),T(0)]);
    run('next-clears-old-target-buy-'+reversed,initial,[N(base(42,!reversed,7,239))]);
    run('next-target-fields-ignored-then-explicit0-'+reversed,initial,[N(base(42,!reversed,999,239)),T(0)]);
    run('advance-clears-and-resets-left-'+reversed,initial,[G(42)]);
    run('advance0-then-target7-'+reversed,initial,[G(0),T(7)]);
}
run('explicit0-after-fresh-target0',base(7,false,0,235),[T(0)]);
run('explicit0-equals-source-highlights-left',base(0,false,0,235),[T(0)]);
run('targets-without-an-attack-preserve-turn-source',null,[T(0),T(7),T(42)]);
run('two-advances-increment-twice',base(),[G(0),G(42)],{gf:0});
run('negativeGF-safe-advance',null,[G(0)],{gf:-1});
run('next-doesnot-restore-positive-buying',null,[N(base(0,false,42,235))]);
run('next-unknown-unused-target-doesnot-lookup',null,[N(base(0,true,999,235))]);
run('next-empty-models-prayer',null,[N(base(0,true,7,235,[])),T(7)]);
run('next-Fate-single-drop',null,[N(base(0,false,7,0,[207,207,6])),T(7)]);
run('next-19-model-column',null,[N(base(0,false,7,0,[6,...Array(18).fill(12)])),T(7)]);
run('next-dynamic112-canonical',null,[N({...base(0,false,7,0,[112]),atk:1,hitRate:25}),T(7)]);
run('next-guardian-dynamic-compound',null,[N({...base(0,true,7,235,[58,12]),guardian:'earth',atk:34,mp:99,cp:-99}),T(7)]);
run('fresh-guardian-then-advance-clears-art',{...base(0,true,7,235,[286]),guardian:'moon'},[G(42)]);
run('fresh-ending-unknown-endpoints-no-lookup',base(999,true,999,235),[],{over:true});
run('ending-next-has-no-ending-gate',base(999,true,999,235),[N(base(0,true,999,235))],{over:true});
run('ending-next-then-explicit-target',base(999,true,999,235),[N(base(0,true,999,235)),T(7)],{over:true});
run('ending-advance-has-no-ending-gate',null,[G(0)],{over:true,turn:999});
run('ending-target-core-before-overlay-error',null,[T(0)],{over:true,turn:999});
run('unknown-next-source-needed-lookup',base(),[N(base(999,false,7,0))]);
run('unknown-explicit-target-needed-lookup',base(),[T(999)]);
run('unknown-advance-partially-clears-before-lookup',base(0,true,7,235),[G(999)]);
run('explicit0-needs-real-roster0',base(7,false,0,0),[T(0)],{roster:[7,42]});
run('fresh-source0-needs-real-roster0',base(0,false,7,0),[],{roster:[7,42]});
run('next-invalid-model-partial-source-draw',base(),[N(base(0,false,7,0,[6,297]))],{adapter_strict_error:true});
run('strict-unshown-buying-domain',base(0,false,0,999),[],{adapter_strict_error:true});
run('next-strict-unshown-buying-domain',base(),[N(base(0,false,999,999))],{adapter_strict_error:true});
run('strict-safeGF-overflow-extension',null,[G(0)],{gf:Number.MAX_SAFE_INTEGER});
const encodeAttack=a=>a===null?'-':[a.playerId??0,({'mars':1,'mercury':2,'jupiter':3,'saturn':4,'uranus':5,'pluto':6,'neptune':7,'venus':8,'earth':9,'moon':10})[a.guardian]??0,a.itemModelIds?.length?a.itemModelIds.join(','):'-',a.atk??0,a.hitRate??0,a.mp??0,a.cp??0,+!!a.reversed,a.targetPlayerId??0,a.buyingItemModelId??0].join(':');
const encodeEvent=e=>e.action==='nextAttack'?'N:'+encodeAttack(e.attack):e.action==='setTargetPlayer'?'T:'+e.playerId:'G:'+e.playerId;
const lines=['# official-active-display-vectors-v50: label roster gf over turn initialattack events expectedJSON'];
for(const c of cases)lines.push([c.label,c.input.roster.length?c.input.roster.join(','):'-',c.input.gf,+c.input.over,c.input.turn,encodeAttack(c.input.initial),c.input.events.length?c.input.events.map(encodeEvent).join('|'):'-',c.expected_native_json].join('\t'));
const tsv=lines.join('\n')+'\n';
for(const c of cases){if(c.label.startsWith('same-source-')){if(c.client_final_or_partial_view.panes.left===null||c.client_final_or_partial_view.panes.right===null||c.client_final_or_partial_view.attack.buying_model!==235)throw new Error('aT same-source erased opposite pane/product');}if(c.label==='next-unknown-unused-target-doesnot-lookup'&&(c.core_error||c.client_final_or_partial_view.attack.target!==null||c.client_final_or_partial_view.attack.buying_model!==null))throw new Error('Incremental nextAttack consumed fresh-only target/buying fields');}
const counts={sequences:cases.length,event_calls:cases.reduce((n,c)=>n+c.steps.length,0),full_client_errors:cases.filter(c=>c.full_client_error).length,core_errors:cases.filter(c=>c.core_error).length,native_error_cases:cases.filter(c=>c.adapter_expected_error).length,native_tsv_vectors:cases.length};
const report={schema_version:1,title:'Actual nextAttack/setTargetPlayer/advanceGF active-display reception v50',source:manifest.client_url,client_sha256:manifest.client_sha256,cards_sha256:manifest.cards_sha256,source_helper_sha256:helperHashes,reused_harness:{path:'scripts/official_active_attack_oracle.cjs',sha256:helperSHA,prefix_sha256:digest(helperText.slice(0,cut)),boundary:'Only factory declarations before v48 case generation are evaluated;no old archive is rewritten. Actual helpers execute in a fresh pinned client VM.'},methods:['A.to/p6/tn actual event/attack/fresh decoders','ks.k complete nextAttack,setTargetPlayer,advanceGF branches with synchronous Future shell','fs.fu/aa actual frame build/clear;fv.fq actual fresh caller;fv.aT actual target-pane update','h8.u/J actual roster lookup;fs.a6/aO actual model-column access','dA.iI/gf2 actual summary and v48 public card numeric projection'],projection_scope:'Roster IDs/GF/panes/selection/active summary and drawn card values only;not board resources,hand,curse,guardian effects,actor/server phase,controls,animations or timers.',vectors:{path:'data/official-v2/active-display-vectors-v50.tsv',bytes:Buffer.byteLength(tsv),sha256:digest(tsv),cases:cases.length,columns:['label','roster','gf','over','turn','initialattack','events','expectedJSONorERROR'],attack_encoding:'source:guardianEnum:modelsCSV-or-:atk:rate:mp:cp:reversed:target:buying;initial- means no initial frame',event_encoding:'N:<attack>|T:<playerId>|G:<playerId>;events- means empty sequence'},counts,cases,conclusions:['Incremental nextAttack invokes fu after source lookup,clears both panes/selection/product/old frame,and draws source plus the supplied column. It doesnot replay the fresh-only positive target/buying restoration. Even unknown positive rawtarget is ignored by this incremental branch.','setTargetPlayer performs a real roster lookup even for ID0. aT selecting the current source only highlights sourceSide;it doesnot clear the old opposite player or commodity card. A different target updates the opposite pane while preserving a drawn commodity.','advanceGF calls actual aa,which resets orientation/panes/selection/old frame,then increments GF and looks up the supplied player for the left source pane. Unknown event player can therefore throw after client partial clear/increment. Native atomic rejection is a separate adapter guarantee.','Fresh isOver initialization skips turn/source/target lookups and leaves the newly constructed active display empty. The examined nextAttack and advanceGF event branches have no equivalent ending gate and can update that display afterward.','A target event on a completely empty ending display can update a pane/selection before later overlay code dereferences the missing source. Core pane changes and complete-handler visual errors are therefore recorded separately. Native partial display projection is not an emulator of those overlay failures.','No event is generated here. The supplied sequence determines every update;there is no RNG,server damage,recipient selection,copy/gift policy,phase transition or implicit action.'],limitations:['This report tests a bounded display reducer,not Match import/replay or an authoritative event stream. Initial fresh construction uses spectator/no-Fog/no-Flash board shells;hand/resources/UID/rating/control widgets and timed visual overlays are outside scope.','Complete handler errors retain the actual final-or-partial display in JSON for diagnosis. ERROR TSV rows indicate native whole-batch rejection and do NOT assert that the browser rolled back its earlier visual changes.','The adapter rejects unsafe GF+1 and invalid knownmodel fields even when that raw field would remain undrawn. These strict-domain extensions are labeled separately from actual handler lookup failures.','For a full-handler error after successful core pane update,the expected native JSON represents only that completed core projection. Following events are not invented after a failed handler in these fixtures.','The v48 attack serializer semantics are retained except target comes from current selected pane and an already drawn commodity persists across T events. Neither rawunusedscalar nor a future model/event queue is exposed.','The reused factory and helper hashes are pinned;no client source text is embedded in the report. No applicationstartup,network/account,timer,C++engine/build or existing source/report edits run.'],network_or_application_startup:false,generator:'node scripts/official_active_display_oracle.cjs --client /path/main.dart.js [--check] [--vectors]'};
const output=JSON.stringify(report,null,2)+'\n';if(check&&(fs.readFileSync(path.join(directory,'active-display-client-v50.json'),'utf8')!==output||fs.readFileSync(path.join(directory,'active-display-vectors-v50.tsv'),'utf8')!==tsv))throw new Error('Active display report/vectors differ');process.stdout.write(vectorsOnly?tsv:output);
