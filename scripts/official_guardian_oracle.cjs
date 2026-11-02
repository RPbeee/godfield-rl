#!/usr/bin/env node
// Actual public guardian attack decoding/display/predicates, never event scheduling.
// stdout only; --check compares the archived report without writing files.
'use strict';
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm');
const {loadPinnedClient,digest}=require('./official_client_oracle.cjs');
const args=process.argv.slice(2);let client=null,check=false;
for(let i=0;i<args.length;++i){if(args[i]==='--check'&&!check)check=true;else if(args[i]==='--client'&&client===null&&i+1<args.length&&!args[i+1].startsWith('--'))client=args[++i];else throw new Error('Usage: node scripts/official_guardian_oracle.cjs --client PATH [--check]');}
if(client===null)throw new Error('--client is required');
const directory=path.resolve(__dirname,'../data/official-v2');
const {A,B,t,$,box,binding,manifest,originalSource}=loadPinnedClient(['--client',client,'--catalog-dir',directory],'scripts/official_guardian_oracle.cjs');
if(binding.version!=='official-v2')throw new Error('Guardian audit requires the pinned official-v2 pair');
const facts=JSON.parse(fs.readFileSync(path.join(directory,'cards.json'),'utf8'));
function helperHash(startMarker,endMarker){const s=originalSource.indexOf(startMarker),e=originalSource.indexOf(endMarker,s);if(s<0||e<s||originalSource.indexOf(startMarker,s+1)>=0)throw new Error('Pinned helper changed');return digest(originalSource.slice(s,e));}
const helperHashes={attack_decoder:helperHash('p6(a){var s,r,q,p,o,n,m,l,k,j=a.i(0,"playerId")','\npn(a){'),frame_projection:helperHash('fu(a,b){var s,r,q,p,o,n,m,l,k,j,i,h,g=this','\naa(a){'),summary:helperHash('iI(a,b,c,d){var s,r,q=this,p=$.r.v(),o=q.d,n=o.length','\ngf2(){'),attack_flag:helperHash('gf2(){var s=B.c.gI(this.d)','\nA.fl.prototype={')};
const decodeScript=new vm.Script(`oracle.A.p6(oracle.A.${binding.decodeJSON}(JSON.parse(attackText)))`);
function decode(raw){box.attackText=JSON.stringify(raw);return decodeScript.runInContext(box);}
function decoded(a){return {playerId:a.a,guardian:a.b===null?null:a.b.c,itemModelIds:Array.from(a.c),atk:a.d,hitRate:a.e,mp:a.f,cp:a.r,targetPlayerId:a.w,reversed:a.x,buyingItemModelId:a.y};}
const decoderInputs=[{}, {guardian:'mars',itemModelIds:[245]}, {playerId:0,guardian:'moon',itemModelIds:[286,235],atk:17,hitRate:60,mp:23,cp:31,targetPlayerId:0,reversed:true,buyingItemModelId:235}, {guardian:'unknown',itemModelIds:[285]}, {playerId:'17',guardian:7,itemModelIds:[],atk:'3',hitRate:null,mp:true,cp:'4',targetPlayerId:'7',reversed:1,buyingItemModelId:'235'}, {playerId:17,guardian:'neptune',itemModelIds:[275],atk:-1,hitRate:-2,mp:-3,cp:-4,targetPlayerId:7}, {itemModelIds:['245']}, {guardian:'mars',itemModelIds:[0,999]}];
const decoderCases=decoderInputs.map(input=>{try{return {input,result:'decoded',output:decoded(decode(input))};}catch(_){return {input,result:'rejected'};}});
const visible=id=>Object.assign(Object.create(A.aC.prototype),{e:$.p[id-1],f:null,w:null});
const selected=ids=>A.u(ids.map(visible),t.k);
const responseIDs=[38,39,189,190,233,234,118,115,116,117,119,148,149,178];
$.t=()=>({z:'offline-viewer'});
function accepted(prefix,next,display){return A[binding.selection].prototype[binding.append].call({r:true,a:{x:{J:()=>({ay:{ak:()=>false}})},f:{gaW:()=>display}}},selected(prefix),visible(next));}
const originalCard=A.a4,originalSummary=A.aR;
const cases=[];
// Execute summary rendering with only text/CSS sinks replaced; no DOM is created.
$.r={v:()=>({P:key=>key,eU:(power,rate)=>[power,rate]})};
function execute(label,guardian,ids,stats){
 const input={playerId:17,guardian,itemModelIds:ids,atk:stats[0],hitRate:stats[1],mp:stats[2],cp:stats[3],targetPlayerId:7,reversed:stats[4],buyingItemModelId:0};
 const attack=decode(input),wire=decoded(attack),widgets=[],source=[],guardians=[],summaries=[];
 A.a4=(card,atk,plus,cp,curse,disguised,rate,mp)=>{const w=[card.a,atk,+plus,cp,curse===null?'-':curse.c,+disguised,rate,mp];widgets.push(w);return w;};
 A.aR=(cards,filter,atk,rate)=>Object.assign(Object.create(A.dA.prototype),{d:cards,e:atk,f:rate,r:filter});
 const frame={aa(){},az(side,player){source.push([side.a,player.c]);},bk(g){guardians.push(g===null?'-':g.c);},af(){},fp(){},a_(side,display){summaries.push(display);}};
 try{A.fs.prototype.fu.call(frame,attack,{c:wire.playerId});}finally{A.a4=originalCard;A.aR=originalSummary;}
 if(source.length!==1||source[0][1]!==wire.playerId||summaries.length!==1||widgets.length!==ids.length||guardians[0]!==guardian)throw new Error('Frame projection changed');
 const display=summaries[0],first=display.d[0],category=first.y==='categoryWeapons'?B.j:first.d;
 let summary=null;Object.assign(display,{sq(){},hl(){},cC(value){summary=value;}});
 A.dA.prototype.iI.call(display,null,null,null,null);
 const element=A[binding.attackElement](display.d),power=A[binding.attack](display.d,wire.atk),attackFlag=display[binding.attackFlag]();
 const mask=prefix=>responseIDs.reduce((n,id,i)=>n+(accepted(prefix,id,display)?2**i:0),0);
 cases.push([label,guardian,ids.join(','),wire.atk,wire.hitRate,wire.mp,wire.cp,+wire.reversed,source[0][0],category.c,element===null?'-':element.c,+attackFlag,power,Array.isArray(summary)?summary[1]:null,A[binding.cost](display.d),+first.gkM(),+first.glj(),widgets.map(w=>w.join(':')).join(','),mask([]),+accepted([],189,display),mask([189])]);
}
const normal=[0,0,0,0,false],dynamic=[17,60,23,31,false],reversed=[0,75,7,9,true],negative=[-1,-2,-3,-4,false];
const guardianRows=facts.filter(c=>c.category==='guardians');
if(guardianRows.length!==42)throw new Error('Guardian fact table changed');
for(const c of guardianRows)for(const stats of [normal,dynamic,reversed,negative])execute('guardian-table',c.guardian,[c.id],stats);
for(const c of facts.filter(c=>['trade','weapons','armor','sundries','miracles'].includes(c.category))){execute('ordinary-single-model',c.category==='miracles'?'moon':'earth',[c.id],normal);if(c.category==='miracles')execute('hypothetical-moon-compound','moon',[286,c.id],normal);else execute('hypothetical-earth-drop','earth',[285,c.id],normal);}
for(const id of [58,112,181,184,187,188])for(const stats of [dynamic,reversed,negative])execute('ordinary-dynamic-display','earth',[id],stats);
const ownerOnly=Object.assign(Object.create(A.c5.prototype),{ch:{l(){}} ,z:{a:{append(){}}}});
const originalGuardian=A.uy;const guardianChanges=[];
A.uy=g=>({a:{},l(){}});
try{for(const g of B.a8){A.c5.prototype.sd0.call(ownerOnly,g);guardianChanges.push(ownerOnly.ch!==null);}A.c5.prototype.sd0.call(ownerOnly,null);guardianChanges.push(ownerOnly.ch===null);}finally{A.uy=originalGuardian;}
if(!guardianChanges.every(Boolean))throw new Error('Guardian display setter changed');
if(cases.length!==664||guardianChanges.length!==11||decoderCases.length!==8)throw new Error('Guardian audit coverage changed');
const report={schema_version:1,scope:'public-client-guardian-attack-projection;not-server-selection-or-schedule',source:manifest.client_url,client_sha256:manifest.client_sha256,cards_sha256:manifest.cards_sha256,methods:['A.p6 attack decoder','fs.fu received-frame projection (visual shell/A.a4/A.aR construction stubbed)','dA.gf2 attack flag and iI summary rendering (text/CSS sinks stubbed)','tz/lc/ld arithmetic','jO.eu visible response append','aN.gkM/glj start and default-target predicates','c5.sd0 guardian display replacement (A.uy visual widget stubbed)'],helper_sha256:helperHashes,guardian_action_ids:guardianRows.map(c=>c.id),guardian_action_rows:guardianRows,guardian_action_weight_sums:Object.fromEntries([...new Set(guardianRows.map(c=>c.guardian))].map(g=>[g,guardianRows.filter(c=>c.guardian===g).reduce((n,c)=>n+(c.guardianAttackRate??0),0)])),counts:{decoder_cases:decoderCases.length,projection_cases:cases.length,guardian_table_cases:42*4,ordinary_single_model_cases:239,hypothetical_compound_cases:239,ordinary_dynamic_cases:18,guardian_display_changes:guardianChanges.length},decoder_cases:decoderCases,response_model_bit_order:responseIDs,widget_override_columns:['model','atk','plus_display','cp','curse','disguised','hit_rate','mp'],case_columns:['group','guardian','model_ids_csv','dto_atk','dto_hit_rate','dto_mp','dto_cp','reversed','source_pane','effective_category','attack_element','attack_flag','composed_power','display_hit_rate','composition_mp_cost','manual_main_allowed','enemy_default','widget_overrides_csv','empty_response_mask','filter_prefix_is_legal','after_filter_append_mask'],cases,network_or_application_startup:false,conclusions:['All 42 guardian action rows have raw category guardians. Only categoryWeapons on 264/286 changes effective incoming category to weapons. No automatic miracle category is inferred from a guardian.','Attack flag is base.atk>0 or raw base category weapons or positive DTO atk; it is independent of effective-category override and may differ from arithmetic composed power in hypothetical addItem compounds.','Positive DTO atk overrides the base before ordered additions; zero/negative falls back to base ATK in the public arithmetic helper. Positive DTO hitRate overrides display; zero/negative displays base rate.','DTO mp/cp feed counterBoost2xMP/counterTakeCP item labels. They do not establish a guardian fee, current caster resource, payment, gift or retained-miracle policy.','The attack decoder defaults missing/noninteger targetPlayerId to 0, not null or the source player. A rendering-compatible input still needs explicit validated source/target semantics in the executable adapter.','fs.fu preserves provided source/reversed/guardian/model information for display; target selection and server event ordering are outside this helper. All guardian raw models reject ordinary manual-main initiation.','c5.sd0 replaces/clears an existing displayed guardian without validating summon restrictions or choosing a type. No independent guardian HP, weakness or idle weight is established.'],limitations:['Target selection,25 percent opportunity,10 percent departure,summon type draw,Earth/Moon candidate selection and server phase/queue are intentionally not executed or inferred.','Ordinary-single and compound cases are externally supplied hypothetical payloads used to test the public formula; they are not proof that a guardian selector generates those arrays.','The client decoder accepts some structurally invalid renderer inputs such as 0/unknown model IDs; an executable import must validate known IDs, nonempty shapes, source/target existence and its own closed effect scope.','The after-filter append mask is the raw append predicate with a supplied prefix; filter_prefix_is_legal must also be true before considering a complete response legal.','No DOM, Firebase, account, timer, game connection or Dart application startup runs. Renderer visuals are stubbed, not server transitions.'],generator:'node scripts/official_guardian_oracle.cjs --client /path/to/main.dart.js [--check]'};
const output=JSON.stringify(report,null,2)+'\n';if(check&&fs.readFileSync(path.join(directory,'guardian-client-v31.json'),'utf8')!==output)throw new Error('Guardian client report differs');process.stdout.write(output);
