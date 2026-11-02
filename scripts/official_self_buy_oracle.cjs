#!/usr/bin/env node
// Public client target selection + submit preflight only; never a server BUY engine.
// No file writes: stdout is the reproducible report and --check compares it.
'use strict';
const fs=require('node:fs'),path=require('node:path');
const {loadPinnedClient,digest}=require('./official_client_oracle.cjs');
const args=process.argv.slice(2);let client=null,check=false;
for(let i=0;i<args.length;++i){if(args[i]==='--check'&&!check)check=true;else if(args[i]==='--client'&&client===null&&i+1<args.length&&!args[i+1].startsWith('--'))client=args[++i];else throw new Error('Usage: node scripts/official_self_buy_oracle.cjs --client PATH [--check]');}
if(client===null)throw new Error('--client is required');
const directory=path.resolve(__dirname,'../data/official-v2');
const {A,B,t,$,binding,manifest,originalSource}=loadPinnedClient(['--client',client,'--catalog-dir',directory],'scripts/official_self_buy_oracle.cjs');
if(binding.version!=='official-v2')throw new Error('Self BUY audit requires the pinned official-v2 client/catalog pair');
function sourceHash(startMarker,endMarker){const start=originalSource.indexOf(startMarker),end=originalSource.indexOf(endMarker,start);if(start<0||end<start||originalSource.indexOf(startMarker,start+1)>=0)throw new Error('Pinned source helper changed');return digest(originalSource.slice(start,end));}
const helperHashes={target:sourceHash('aT(a){var s,r,q,p,o','\ncH(a){'),submit:sourceHash('A.jS.prototype={','\nA.jQ.prototype={')};
// Execute the actual async switch body synchronously until its first await.
// Target selection and all preflight/payload construction precede that await.
// Only the Dart Future scheduler, sounds, visual methods and RPC are stubbed.
A.l=()=>({});A.m=fn=>fn;A.k=fn=>fn(0,null);A.b=x=>x;A.j=x=>x;A.i=x=>{throw x;};
const cases=[];
function run(ownerId,mp,cp,fog,dream,targetKind,repeat){
 const sounds=[],visuals=[],wire=[];let cpReads=0;
 A.o=s=>sounds.push(s);
 const player=(id,user,team,dead=false)=>Object.assign(Object.create(A.c5.prototype),{c:id,d:user,r:team,x:dead,CW:null,at:{gK:()=>String(mp)},ay:{ak:curse=>fog&&curse===B.q||dream&&curse===B.K},si1(){}});
 const own=player(ownerId,'offline-viewer',B.ad),enemy=player(7,'enemy',B.ae),ally=player(9,'ally',B.ad),enemy2=player(42,'enemy2',B.af),dead=player(55,'dead',B.ae,true),unknown=player(99,'not-in-roster',B.ae);
 Object.defineProperty(own,'ax',{get(){++cpReads;throw new Error('Unexpected CP read in BUY submit');}});
 const roster=new A.h8(A.u([own,enemy,ally,enemy2,dead],t.r),null);
 const target={self:own,enemy,ally,dead,unknown,none:null}[targetKind];
 const capture=Object.assign(Object.create(A.H.prototype),{b2(method,flag,outer){const command=outer.i(0,'command');const payload={itemIds:Array.from(command.i(0,'itemIds'))};for(const key of ['targetPlayerId','mp','cp'])if(command.ak(key))payload[key]=command.i(0,key);wire.push({method,flag,command:payload});return 'captured-before-network';}});
 $.t=()=>({z:'offline-viewer',w:capture});
 const frame={Q:false,az(side,p){visuals.push(['set-pane',side.a,p===null?null:p.c]);},fz(side,p,hidden){visuals.push(['foreign-pane',side.a,p.c,hidden]);},b5(side){visuals.push(['arrow',side===null?null:side.a]);},au(){},a_(){}};
 const inventory={ed(id){visuals.push(['consume-selection-visual',id]);},fi(){},ar(){}};
 const selected=Object.assign(Object.create(A.aC.prototype),{c:101,e:$.p[4],f:null,w:null});
 const control=Object.assign(Object.create(A.jO.prototype),{a:{x:roster,f:frame,y:inventory,w:{ep(){}}},r:false,c:null,e:null,f:A.u([selected],t.k),eI(){},cH(reason){throw new Error('Unexpected BUY preflight alert:'+reason);}});
 A.jO.prototype.aT.call(control,target);
 const beforeRepeat={sounds:sounds.length,visuals:visuals.length};
 if(repeat)A.jO.prototype.aT.call(control,target);
 const repeatedTargetWasNoop=!repeat||(sounds.length===beforeRepeat.sounds&&visuals.length===beforeRepeat.visuals);
 const retainedTarget=control.e===target;
 const result=new A.jS(control).$0();
 if(result!=='captured-before-network'||wire.length!==1||wire[0].command.itemIds.length!==1||wire[0].command.itemIds[0]!==101||(wire[0].command.targetPlayerId??null)!==(target===null?null:target.c)||cpReads||!retainedTarget||!repeatedTargetWasNoop||control.e!==null||control.f.length)throw new Error('Client BUY submit contract changed');
 cases.push({input:{owner_id:ownerId,mp,cp,fog,dream,target_kind:targetKind,repeat_same_target:repeat},target_id:target===null?null:target.c,target_is_in_alive_click_roster:target!==null&&roster.aG().includes(target),direct_target_method_retained:retainedTarget,same_target_reselection_noop:repeatedTargetWasNoop,cp_reads:cpReads,wire:wire[0],selection_and_target_cleared_after_submit:control.e===null&&control.f.length===0,sounds,visuals});
}
for(const target of ['self','enemy','ally','dead','unknown','none'])for(const repeat of [false,true])run(0,0,0,false,false,target,repeat);
for(const owner of [0,77,Number.MAX_SAFE_INTEGER])for(const mp of [0,99])for(const cp of [0,99])for(const fog of [false,true])for(const dream of [false,true])for(const target of ['self','enemy'])run(owner,mp,cp,fog,dream,target,true);
const report={schema_version:1,scope:'public-client-target-and-submit-only;not-server-buy-transitions',source:manifest.client_url,client_sha256:manifest.client_sha256,cards_sha256:manifest.cards_sha256,methods:['jO.aT target selection','jS.$0 submit preflight until first await','h8.aG alive click roster','c5.ce team relationship','A.ld magic cost'],helper_sha256:helperHashes,counts:{cases:cases.length,captured_rpcs:cases.length,cp_reads:cases.reduce((n,c)=>n+c.cp_reads,0)},cases,network_or_application_startup:false,conclusions:['A single selected BUY model5 is submitted with physical itemIds and an explicitly selected self targetPlayerId, including player0 and the maximum safe integer ID.','The selected BUY costs0MP in this preflight; CP is not read. MP0, CP0, Fog and Dream do not insert a self-BUY rejection here.','jO.aT directly retains self/enemy/ally objects and repeated reference-identical selection is a no-op. It also lacks its own dead/unknown-object validation, but those objects are excluded from the normal alive roster or are absent from it.','Actual dead-player or unknown-target server acceptance is not established. The direct method and forced-submit cases are not a claim that ordinary UI clicks make those targets legal.'],limitations:['Visual operations, sound, Future scheduling and the backend RPC are stubbed. Actual target/preflight/command construction code runs unchanged through the first await; no server response path runs.','This does not establish self-BUY commodity choice, price affordability, costs, gifts, consume/return/escrow/quote, payment, reveal, death or cutoff schedule. Those are explicit experimental engine contracts.','The input cp value is an annotation, not a wire field. Its accessor deliberately throws on read to prove this specific submit path does not consult CP.','The report does not establish full official legality or equivalence for other trade commands or future client/catalog versions.'],generator:'node scripts/official_self_buy_oracle.cjs --client /path/to/main.dart.js [--check]'};
const output=JSON.stringify(report,null,2)+'\n';if(check&&fs.readFileSync(path.join(directory,'self-buy-client-v30.json'),'utf8')!==output)throw new Error('Self BUY client report differs');process.stdout.write(output);
