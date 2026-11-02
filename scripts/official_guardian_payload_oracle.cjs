#!/usr/bin/env node
// Actual pinned client received guardian payload projection, not a server selector.
// stdout only; --check compares the separately archived report without file writes.
'use strict';
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm');
const {loadPinnedClient,digest}=require('./official_client_oracle.cjs');
let client=null,check=false;const args=process.argv.slice(2);
for(let i=0;i<args.length;++i){if(args[i]==='--check'&&!check)check=true;else if(args[i]==='--client'&&client===null&&i+1<args.length&&!args[i+1].startsWith('--'))client=args[++i];else throw new Error('Usage: node scripts/official_guardian_payload_oracle.cjs --client PATH [--check]');}
if(client===null)throw new Error('--client is required');
const directory=path.resolve(__dirname,'../data/official-v2');
const {A,B,t,$,box,binding,manifest,originalSource}=loadPinnedClient(['--client',client,'--catalog-dir',directory],'scripts/official_guardian_payload_oracle.cjs');
if(binding.version!=='official-v2')throw new Error('Guardian payload audit requires pinned official-v2');
const facts=JSON.parse(fs.readFileSync(path.join(directory,'cards.json'),'utf8'));
function sourceHash(startMarker,endMarker){const a=originalSource.indexOf(startMarker),b=originalSource.indexOf(endMarker,a);if(a<0||b<a||originalSource.indexOf(startMarker,a+1)>=0)throw new Error('Pinned client helper changed');return digest(originalSource.slice(a,b));}
const helperHashes={guardian_receiver:sourceHash('case 18:s="attackByGuardian"===o?36:37','case 37:s="useAttackItems"===o?47:48'),ordinary_payment_and_inventory:sourceHash('case 55:s=a4?63:64','case 48:s="pray"===o?69:70'),frame_projection:sourceHash('fu(a,b){var s,r,q,p,o,n,m,l,k,j,i,h,g=this','\naa(a){'),summary:sourceHash('iI(a,b,c,d){var s,r,q=this,p=$.r.v(),o=q.d,n=o.length','\ngf2(){')};
const decodeScript=new vm.Script(`oracle.A.p6(oracle.A.${binding.decodeJSON}(JSON.parse(attackText)))`);
const decode=x=>{box.attackText=JSON.stringify(x);return decodeScript.runInContext(box);};
const typed=ids=>A.u(ids.map(id=>$.p[id-1]),t.G);
const visible=id=>Object.assign(Object.create(A.aC.prototype),{e:$.p[id-1],f:null,w:null});
const selected=ids=>A.u(ids.map(visible),t.k);
$.t=()=>({z:'offline-viewer'});$.r={v:()=>({P:x=>x,eU:(power,rate)=>[power,rate]})};
const append=(ids,id)=>A[binding.selection].prototype[binding.append].call({r:false},selected(ids),visible(id));
const start=id=>append([],id);
const supported=facts.filter(c=>c.giftRate>0&&c.id!==207).map(c=>c.id);
const earthBases=supported.filter(id=>['weapons','sundries','armor'].includes(facts[id-1].category)&&id!==94&&start(id));
const moonBases=supported.filter(id=>facts[id-1].category==='miracles'&&start(id));
if(supported.length!==236||earthBases.length!==120||moonBases.length!==26)throw new Error('Payload domain fingerprint changed');
const reactionIDs=[38,39,189,190,233,234,118,115,116,117,119,148,149,178];
function response(prefix,id,display){return A[binding.selection].prototype[binding.append].call({r:true,a:{x:{J:()=>({ay:{ak:()=>false}})},f:{gaW:()=>display}}},selected(prefix),visible(id));}
function localValidation(guardian,ids){
 const known=ids.every(id=>supported.includes(id));
 const domain=known&&ids.every(id=>guardian==='earth'?['trade','weapons','armor','sundries'].includes(facts[id-1].category):facts[id-1].category==='miracles');
 const base=ids.length>0&&(guardian==='earth'?earthBases:moonBases).includes(ids[0]);
 const prefix=[];const appendSteps=ids.map(id=>{const allowed=append(prefix,id);prefix.push(id);return allowed;});
 return {known_supported_models:known,domain_ok:domain,base_ok:base,capacity_ok:ids.length>=1&&ids.length<=18,client_append_steps:appendSteps,client_full_append:appendSteps.every(Boolean),adapter_accepted:known&&domain&&base&&ids.length>=1&&ids.length<=18&&appendSteps.every(Boolean)};
}
// Complete this one received event using the real async switch body. Only the
// Future scheduler, sound, visual/widget constructors and animation sinks are stubbed.
A.l=()=>({});A.m=fn=>fn;A.k=fn=>fn(0,null);A.b=(x,fn)=>fn(0,x);A.j=x=>x;A.i=x=>{throw x;};A.o=()=>{};
const cardBuilder=A.a4,summaryBuilder=A.aR;
A.a4=(card,atk,plus,cp,curse,disguised,rate,mp)=>[card.a,atk,+plus,cp,curse===null?null:curse.c,+disguised,rate,mp];
A.aR=(cards,filtered,atk,rate)=>Object.assign(Object.create(A.dA.prototype),{d:cards,e:atk,f:rate,r:filtered});
function received(guardian,ids,sourceMP){
 let mp=String(sourceMP),mpReads=0,mpWrites=0,feeCalls=0,inventoryCalls=0;const previews=[],widgets=[],identity=[];
 const source={c:17,ch:{c:A.fx(guardian)},at:{gK(){++mpReads;return mp;},a:{get textContent(){return mp;},set textContent(x){++mpWrites;mp=String(x);}}},dG(){++feeCalls;throw new Error('Unexpected generic guardian MP payment');},hF(){++inventoryCalls;throw new Error('Unexpected guardian inventory use');},e7(){++inventoryCalls;throw new Error('Unexpected guardian discard');}};
 const frame={Q:false,aa(){},az(side,p){identity.push(['source',side.a,p.c]);},bk(g){identity.push(['guardian',g===null?null:g.c]);},af(side,w){if(w!==null)widgets.push(w);},aL(){},a_(side,display){previews.push(display);},l7(){return{cq(){}};}};
 const controller=Object.assign(Object.create(A.ks.prototype),{a:{f:frame,x:{u:id=>{if(id!==17)throw new Error('Unexpected source ID');return source;}},A(){return null;},r:{dC(){return null;}}}});
 const event={a:'attackByGuardian',c:decode({playerId:17,guardian,itemModelIds:ids,targetPlayerId:7})};
 A.ks.prototype.k.call(controller,event);
 if(previews.length<ids.length||widgets.length!==ids.length||identity[0]?.[2]!==17||identity[1]?.[1]!==guardian||feeCalls||inventoryCalls)throw new Error('Received guardian projection changed');
 const display=previews.at(-1),first=display.d[0];let rendered=null;Object.assign(display,{sq(){},hl(){},cC(x){rendered=x;}});A.dA.prototype.iI.call(display,null,null,null,null);
 const mask=prefix=>reactionIDs.reduce((n,id,i)=>n+(response(prefix,id,display)?2**i:0),0);
 return {source_player:17,source_guardian:guardian,source_mp_before:sourceMP,source_mp_after:Number(mp),source_mp_reads:mpReads,source_mp_writes:mpWrites,generic_mp_fee_calls:feeCalls,inventory_use_calls:inventoryCalls,received_model_ids:ids,raw_category:first.d.c,effective_category:(first.y==='categoryWeapons'?B.j:first.d).c,attack_flag:display[binding.attackFlag](),power:A[binding.attack](display.d,display.e),element:A[binding.attackElement](display.d)?.c??null,composition_mp_cost:A[binding.cost](display.d),summary:rendered,widget_overrides:widgets,empty_response_mask:mask([]),filter_prefix_is_legal:response([],189,display),after_filter_append_mask:mask([189])};
}
const cases=[];
function run(label,guardian,ids,sourceMP=17,expected=true){const validation=localValidation(guardian,ids);if(validation.adapter_accepted!==expected)throw new Error('Unexpected adapter admission: '+label);const projection=received(guardian,ids,sourceMP);if(ids[0]===58){if(projection.source_mp_after!==0||projection.source_mp_reads!==1||projection.source_mp_writes!==1)throw new Error('Actual MP-stick guardian receiver changed');}else if(projection.source_mp_after!==sourceMP||projection.source_mp_reads||projection.source_mp_writes)throw new Error('Unexpected source-resource consultation');cases.push({label,guardian,models:ids,validation,projection});}
for(const id of earthBases)run('earth-base','earth',[id]);
for(const id of moonBases)run('moon-base','moon',[id]);
const compounds=[[6,12],[6,66],[6,66,83],[58,12],[58,203],[14,12,203],[27,12],[53,66,203],[6,...Array(17).fill(12)],[58,66,83,12,203]];
for(const ids of compounds)run('earth-compound','earth',ids);
for(const [guardian,ids]of [['moon',[210,231]],['moon',[210,232]],['moon',[218,204]],['earth',[6,210]],['earth',[6,231]],['earth',[112,12]],['earth',[94]],['earth',[3]],['earth',[113]],['earth',[208]],['moon',[233]],['earth',[6,...Array(18).fill(12)]],['earth',[285,6]],['moon',[286,210]]])run('rejected-shape-probe',guardian,ids,17,false);
for(const mp of [0,1,99])run('mana-source-boundary','earth',[58,203],mp);
A.a4=cardBuilder;A.aR=summaryBuilder;
const report={schema_version:1,scope:'public-client-projection;experimental-external-guardian-regular-payload-v33;not-selector-or-server-transition-equivalence',source:manifest.client_url,client_sha256:manifest.client_sha256,cards_sha256:manifest.cards_sha256,helper_sha256:helperHashes,methods:['A.p6 decoder','ks.k attackByGuardian complete received-event branch with synchronous Future/visual shells','jO.eu unchanged main and response append','dA.gf2/iI unchanged attack flag/summary','tz/lc/ld unchanged composition arithmetic'],source_identity_rule:'guardian identity and source player are separate from ordinary first-model category and physical inventory tokens',local_adapter_contract:{earth_base_ids:earthBases,moon_base_ids:moonBases,model_capacity:18,whole_sequence_domain:true,append_predicate:'actual-client-main-append-for-every-prefix',target:'explicit-live-single-target;14-repeats-same-target;percentage-rolls-once-for-chosen-target',cost:'one-external-decision;no-auto-followup-or-GF-or-owner-tick;no-source-items-or-ordinary-MP-fee-or-operation-gift;first58-only-drains-current-MP',unsupported:'general armor, passive, trade and danger bases; cross-domain extras; Moon addon-only231/232 and FREE204; Earth miracle extras;285/286 wrapper prefixes; Earth/Moon selector'},counts:{earth_bases:earthBases.length,moon_bases:moonBases.length,standalone_base_cases:146,accepted_earth_compounds:compounds.length,rejected_shape_cases:14,mana_boundary_cases:3,total_cases:cases.length},response_model_bit_order:reactionIDs,widget_override_columns:['model','atk','plus_display','cp','curse','disguised','hit_rate','mp'],cases,limitations:['Adapter admission is an explicit narrow local contract, not an official server validity/drop/selection algorithm. Guardian-model identity is not a raw-category override.','The actual received-event path draws supplied model IDs without physical-token selection, generic magic payment or inventory retention calls. Only first58 reads source displayedMP, previews2xMP and displaysMP0; this does not establish the authoritative server fee or retention policy.','An ordinary miracle base remains category miracles with miracle reaction matching, and an ordinary weapon base remains category weapons; neither inherits category guardians merely from source guardian art.','Moon[210,231]/[210,232] are rejected by actual main append. Moon[218,204] and Earth[6,210] pass client append but fail the declared whole-sequence guardian domain. The19-model probe passes append but fails the local18-capacity policy.','285/286 hypothetical wrappers are not used: their client display behavior does not prove how Earth/Moon chooses or wraps ordinary models.','Selector weights, eligible pools, summon/opportunity/departure scheduling, ownership, empty-slot dependence, target defaults, authoritative effect resolution and random outcomes are not established by this oracle.','No DOM, Firebase, account, timer, backend network or Dart application startup runs. Text/widget/animation and Future scheduling are stubbed; public decoder, receive-event branch, arithmetic and selection predicates execute unchanged.'],network_or_application_startup:false,generator:'node scripts/official_guardian_payload_oracle.cjs --client /path/to/main.dart.js [--check]'};
if(cases.length!==173)throw new Error('Payload coverage changed');
const output=JSON.stringify(report,null,2)+'\n';if(check&&fs.readFileSync(path.join(directory,'guardian-payload-client-v33.json'),'utf8')!==output)throw new Error('Guardian payload report differs');process.stdout.write(output);
