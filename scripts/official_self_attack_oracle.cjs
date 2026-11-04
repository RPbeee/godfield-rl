#!/usr/bin/env node
// Actual pinned client selection/submit and independent received-frame routing.
// No server simulation or network; stdout JSON, --check compares saved bytes.
'use strict';
const fs=require('node:fs'),path=require('node:path');
const {digest}=require('./official_client_oracle.cjs');
let client=null,check=false;const args=process.argv.slice(2);
for(let i=0;i<args.length;++i){if(args[i]==='--check'&&!check)check=true;else if(args[i]==='--client'&&client===null&&i+1<args.length&&!args[i+1].startsWith('--'))client=args[++i];else throw new Error('Usage: node scripts/official_self_attack_oracle.cjs --client PATH [--check]');}
if(client===null)throw new Error('--client is required');
const directory=path.resolve(__dirname,'../data/official-v2');
const helperBytes=fs.readFileSync(path.join(__dirname,'official_active_attack_oracle.cjs'));
const factorySHA='118f2e4414adde5062eef22be62228eaf656aa53cdb2ef736be430febfbbdc65';
if(digest(helperBytes)!==factorySHA)throw new Error('Frozen v48 factory changed');
const helper=helperBytes.toString('utf8'),cut=helper.indexOf('const decoderCases=[];');if(cut<0)throw new Error('Missing factory boundary');
const factory=helper.slice(0,cut).replace(/^#![^\n]*\n/,'');
const make=new Function('require','__dirname','process',factory+'\nreturn {A,B,t,$,binding,manifest,originalSource,decode,frame};');
const {A,B,t,$,binding,manifest,originalSource,decode,frame}=make(require,__dirname,{argv:['node','v48-factory','--client',client]});
const cards=JSON.parse(fs.readFileSync(path.join(directory,'cards.json'),'utf8'));
function hash(start,end){const a=originalSource.indexOf(start),b=originalSource.indexOf(end,a);if(a<0||b<a||originalSource.indexOf(start,a+1)>=0)throw new Error('Pinned helper changed');return digest(originalSource.slice(a,b));}
const helpers={append:hash('eu(a,b){var s,r,q,p,o,n,m,l="filterAtkElement"','\nio(a){'),selection:hash('io(a){var s,r,q,p,o,n,m=this,l=m.a','\nkO(a){'),selected_target:hash('aT(a){var s,r,q,p,o','\ncH(a){'),submit:hash('A.jS.prototype={','\nA.jQ.prototype={'),frame:hash('fu(a,b){var s,r,q,p,o,n,m,l,k,j,i,h,g=this','\naa(a){'),clear_frame:hash('aa(a){var s,r,q=this,p=null,o=q.r','\nA.kc.prototype={'),received_target:hash('aT(a){var s=this.f','\nfF(){'),activation:hash('fF(){var s,r,q,p,o,n=this,m="command"','\nct(a){')};
// Future shell stops submit at the first RPC await. All validation and wire
// construction before it execute unchanged; the backend is a local sink.
A.l=()=>({});A.m=fn=>fn;A.k=fn=>fn(0,null);A.b=value=>value;A.j=value=>value;A.i=value=>{throw value;};A.o=()=>{};
const instance=(id,token)=>Object.assign(Object.create(A.aC.prototype),{c:token,e:$.p[id-1],f:null,w:null});
function people(mp){return [0,7].map(id=>Object.assign(Object.create(A.c5.prototype),{c:id,d:'user'+id,r:B.p,x:false,CW:null,at:{gK:()=>String(mp)},ay:{ak:()=>false},si1(value){this.click_active=value;}}));}
function selection(models,mp,target){
 const p=people(mp),roster=new A.h8(A.u(p,t.r),null),selected=models.map((id,i)=>instance(id,i)),prefix=A.u([],t.k),steps=[];
 const control=Object.assign(Object.create(A.jO.prototype),{r:false,f:A.u([],t.k),c:null,e:null,eI(){},cH(reason){this.alert=reason;}});
 for(const item of selected){const allowed=A.jO.prototype.eu.call(control,prefix,item);steps.push({model:item.gL().a,allowed});if(!allowed)break;prefix.push(item);}
 const out={models,mp,target,append:steps,selection_accepted:prefix.length===selected.length,composed_mp_cost:A[binding.cost](A.u(selected.map(x=>x.gL()),t.G)),alert:null,captured_rpc:null};
 if(!out.selection_accepted)return out;
 const wire=[],f={Q:false,az(){},fz(){},b5(){},au(){},a_(){}};
 const app={x:roster,f,y:{ed(){},fi(){},ar(){}},w:{ep(){}}};
 const capture=Object.assign(Object.create(A.H.prototype),{b2(method,flag,outer){const cmd=outer.i(0,'command');wire.push({method,flag,command:{itemIds:Array.from(cmd.i(0,'itemIds')),targetPlayerId:cmd.i(0,'targetPlayerId')}});return 'captured-before-network';}});
 $.t=()=>({z:'user0',w:capture});control.a=app;control.f=A.u(selected,t.k);
 A.jO.prototype.aT.call(control,p.find(x=>x.c===target));out.target_retained=control.e===p.find(x=>x.c===target);
 new A.jS(control).$0();out.alert=control.alert??null;out.captured_rpc=wire[0]??null;
 if(wire.length>1||Boolean(out.captured_rpc)!==(mp>=out.composed_mp_cost))throw new Error('Unexpected submit readiness');
 if(out.captured_rpc&&(out.captured_rpc.command.targetPlayerId!==target||out.captured_rpc.command.itemIds.join(',')!==models.map((_,i)=>i).join(',')))throw new Error('Submit changed explicit target or tokens');
 return out;
}
const selectionCases=[];
const bases=[6,32,75,213,14,27,45,58,53,216];
for(const model of bases)for(const mp of [0,99])for(const target of [0,7])selectionCases.push(selection([model],mp,target));
for(const models of [[14,12],[27,231],[58,231],[58,231,204],[53,232],[216,204],[216,231],[216,232],[216,12]])for(const mp of [0,99])for(const target of [0,7])selectionCases.push(selection(models,mp,target));
// Only visual widgets are stubbed below. Actual fu/aT/fF inspect the received
// frame, roster and pane/card occupancy; no server-generated frame is invented.
A.l1=()=>({a:{style:{}}});
function routing(models,target,reversed,source=0){
 const p=people(99),roster=new A.h8(A.u(p,t.r),null),f=frame(),columns={a:A.u([],t.c),b:A.u([],t.c)};f.people=p;
 const oldSummary=f.a_;Object.assign(f,{r:null,w:null,z:null,bg(){},au(side){columns[side===B.a?'a':'b'].length=0;},a_(side,summary){if(summary)oldSummary.call(this,side,summary);},aa(){A.fs.prototype.aa.call(this,0);}});
 f.af=function(side,widget){columns[side===B.a?'a':'b'].push({c:{d:$.p[widget.model-1]}});};
 f.a6=side=>columns[side===B.a?'a':'b'];f.aO=side=>A.u(f.a6(side).map(x=>x.c.d),t.G);
 const activation=[];const viewer=target;
 const app=Object.assign(Object.create(A.fv.prototype),{f,x:roster,w:{iv(){activation.push('viewer-active');},iw(person){activation.push('waiting:'+person.c);}},e:{gbw:()=>false},y:{a:{append(){}},ec(){}},Q:{f:A.u([],t.k),r:null,eI(){},ds(){},fv(){activation.push('purchase');}}});app.Q.a=app;
 const capture=Object.assign(Object.create(A.H.prototype),{ch:app});$.t=()=>({z:'user'+viewer,w:capture});
 const raw={playerId:source,itemModelIds:models,targetPlayerId:target,reversed,atk:models[0]===58?34:0};
 A.fs.prototype.fu.call(f,decode(raw),p.find(x=>x.c===source));A.fv.prototype.aT.call(app,p.find(x=>x.c===target));
 const beforeActivation={...f.panes};A.fv.prototype.fF.call(app);
 const out={supplied_received_frame:raw,viewer,panes_before_activation:beforeActivation,panes_after_activation:{...f.panes},command_response:app.Q.r,activation,alive_click_targets:p.filter(x=>x.click_active).map(x=>x.c)};
 if(out.command_response!==(source!==target))throw new Error('Unexpected self/foreign pane branch');
 if(source===target&&out.alive_click_targets.join(',')!=='0,7')throw new Error('Main activation did not enable own and foreign alive targets');
 return out;
}
const routingCases=[];
for(const models of [...bases.map(id=>[id]),[14,12],[27,231],[58,231],[58,231,204],[53,232],[216,204]])for(const target of [0,7])for(const reversed of [false,true])routingCases.push(routing(models,target,reversed));
routingCases.push(routing([14],7,false,7));
const ja=JSON.parse(fs.readFileSync(client.replace(/main\.dart\.js$/,'ja.json'),'utf8'));
if(digest(fs.readFileSync(client.replace(/main\.dart\.js$/,'ja.json')))!==manifest.source_raw_sha256)throw new Error('Companion JA pin differs');
const publicGetReview={"kind":"Separate public GET review, not a network operation performed by this offline oracle.","assets":[{"url":"https://godfield.net/main.dart.js","retrieved_at_utc":"2026-11-04T11:38:03.984751+00:00","status":"200","curl_exit":0,"stderr":"","bytes":440420,"sha256":"5ece06c224b6a7a5d877eb45d22ea18b82a03aaf1b7161cbbdf6a1e23ceab2cd","equal_pin":true,"validators":{"date":"Wed, 04 Nov 2026 11:38:04 GMT","etag":"\"eee0fd31f9de4c2975db90e00685294056d09407308ba3e0249dbd40b3c47998\"","last-modified":"Wed, 04 Nov 2026 07:33:42 GMT"}},{"url":"https://godfield.net/i18n/ja.json","retrieved_at_utc":"2026-11-04T11:38:03.988192+00:00","status":"200","curl_exit":0,"stderr":"","bytes":49240,"sha256":"91b82c2600eac771c1a94dec80f249758496e2bbcb7befe6db52dacec1b55b02","equal_pin":true,"validators":{"date":"Wed, 04 Nov 2026 11:38:04 GMT","etag":"\"603e2c27b9ce82df18db251881d9731c5409d4a179d5d8cf4ed8b00d10f9289b\"","last-modified":"Wed, 04 Nov 2026 07:33:42 GMT"}}],"interpretation":"Both complete decoded bodies and ETags match the official-v2 pin. Last-Modified advanced again;this alone does not establish a source or rules change."};
const report={schema_version:1,title:'Actual public-client special self-selection and independently supplied received-frame routing v55',public_get_review:publicGetReview,source:manifest.client_url,client_sha256:manifest.client_sha256,cards_sha256:manifest.cards_sha256,ja:{source:manifest.source_url,sha256:manifest.source_raw_sha256},helper_sha256:helpers,reused_factory:{path:'scripts/official_active_attack_oracle.cjs',sha256:factorySHA,prefix_sha256:digest(helper.slice(0,cut)),scope:'Factory declarations only, before old case generation;actual helpers run in a new pinned VM.'},methods:['actual jO.eu main append and aN.gkM','actual jO.aT selected self/foreign target','actual jS.$0 through first await, with captured submit-command RPC','actual p6 decode,fs.fu/aa,fv.aT and fv.fF on separately supplied received frames','actual h8 roster and fF alive-target si1 activation'],facts:bases.map(id=>cards[id-1]),localized_description:{attackTwice:ja.texts.abilities.attackTwice,absorbHP:ja.texts.abilities.absorbHP,atkBy2xMP:ja.texts.game.atkBy2xMP,consumeAllMP:ja.texts.abilities.consumeAllMP,dealSameDamage:ja.texts.abilities.dealSameDamage},counts:{selection_cases:selectionCases.length,captured_submit:selectionCases.filter(x=>x.captured_rpc).length,append_rejected:selectionCases.filter(x=>!x.selection_accepted).length,mp_alerts:selectionCases.filter(x=>x.alert==='miracles').length,received_routing_cases:routingCases.length,received_self:routingCases.filter(x=>x.supplied_received_frame.playerId===x.supplied_received_frame.targetPlayerId).length,received_foreign:routingCases.filter(x=>x.supplied_received_frame.playerId!==x.supplied_received_frame.targetPlayerId).length},selection_and_submit:selectionCases,independent_received_routing:routingCases,conclusions:['14/27/45/58/53/216 single cards are selectable as ordinary main cards;the actual submit preflight preserves explicitly selected self IDs and physical itemIds when composed MP is affordable. 216 has its normal10MP cost;58 itself has no fixed MP fee in this preflight.','The submitted target binding does not contain a special-card self rejection. Ordinary controls and the special candidates follow the same inspected main-selection/submit path. This does not execute a server acceptance path.','For independently supplied nonempty single frames with source==target,fv.aT highlights the source pane without creating the opposite pane;actual fv.fF chooses main activation,not a separate response command. The inspected routing has no card-ability branch. Reversed orientation and player7 self follow the same law.','Foreign frames preserve the opposite target pane and activate response mode,including every tested special base and legal compound. No claim is made that the server actually delivers a particular supplied frame.','The two report sections are independent probes,not an observed submit-to-server-to-frame trace. The all-enemy compound example records client selection/frame behavior only;it does not resolve the corresponding server target roster.'],live_evidence_boundary:{record:'data/official-v2/live-transition-witnesses-v54.json',completed_public_transition_witnesses:7,routing_only_observations:1,captured_full_server_event_traces:0,new_live_trials_in_this_oracle:0},limitations:['The backend RPC,DOM/sound/widgets and Future scheduler are stubbed;source selection and pane/activation helpers execute unchanged. No authentication,network,application startup or Match executor runs.','14 two-hit framing and inter-hit defense/counter/gift/death order,absorption source healing stage,58 actual MP consumption stage,53 source/target damage order and216 actual effect timing are not established.','This supports client-selectable self targets and the conditional routing of a supplied received frame. It does not prove authoritative server self-attack rules,immediate versus automatic-empty resolution,or general simulator equivalence.','No special self-body execution or damage was measured by this oracle. Existing live completed witnesses remain7;any engine extension to special bases is an explicit limited generalization,not new measured server body evidence.','The separately supplied58 atk34 is a display input,not a proof that a server computed34 from a particular MP value. Future raw frames,repeat segmentation,selector outcomes and hidden inventories are not reconstructed.'],network_or_application_startup:false,generator:'node scripts/official_self_attack_oracle.cjs --client /path/to/main.dart.js [--check]'};
const output=JSON.stringify(report,null,2)+'\n';if(check&&fs.readFileSync(path.join(directory,'self-attack-client-v55.json'),'utf8')!==output)throw new Error('Self-attack client report differs');process.stdout.write(output);
