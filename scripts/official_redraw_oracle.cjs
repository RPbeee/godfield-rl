#!/usr/bin/env node
// Actual pinned client replaceItems inventory projection, not a server redraw engine.
// JSON goes to stdout only; --check verifies the separately archived report.
'use strict';
const fs=require('node:fs'),path=require('node:path');
const {loadPinnedClient,digest}=require('./official_client_oracle.cjs');
const args=process.argv.slice(2);let client=null,check=false;
for(let i=0;i<args.length;++i){if(args[i]==='--check'&&!check)check=true;else if(args[i]==='--client'&&client===null&&i+1<args.length&&!args[i+1].startsWith('--'))client=args[++i];else throw new Error('Usage: node scripts/official_redraw_oracle.cjs --client PATH [--check]');}
if(client===null)throw new Error('--client is required');
const directory=path.resolve(__dirname,'../data/official-v2');
const {A,t,$,binding,manifest,originalSource}=loadPinnedClient(['--client',client,'--catalog-dir',directory],'scripts/official_redraw_oracle.cjs');
if(binding.version!=='official-v2')throw new Error('Redraw audit requires the pinned official-v2 client/catalog pair');
const start=originalSource.indexOf('lI(a){var s,r,q,p,o,n,m,l,k,j');
const end=originalSource.indexOf('\nglb(){',start);
if(start<0||end<start||originalSource.indexOf('lI(a){var s,r,q,p,o,n,m,l,k,j',start+1)>=0)throw new Error('Pinned inventory replacement helper changed');
const helperSHA=digest(originalSource.slice(start,end));
// The real lI loop and Dart collection operations remain untouched. Only DOM
// casts and visual widget construction are stubbed; input uses real item decoder.
const cases=[];let state=0x6d2b79f5;
const random=n=>{state^=state<<13;state^=state>>>17;state^=state<<5;return(state>>>0)%n;};
const originalCast=A.a,originalBuilder=A.l1;
function run(initial,records){
 const decoded=A.u(records.map(r=>A.l0(A.ag(Object.entries(r).flat(),t.N,t.z))),t.po),removed=[],built=[];
 const slots=A.u(initial.map((v,i)=>Object.assign(Object.create(A.aC.prototype),{c:i+1,e:v.empty?null:$.p[v.used?234:5],w:v.used?{}:null,a:{style:{left:String(i*82)+'px',top:i<9?'88px':'190px'}},l(){removed.push(i);}})),t.k);
 const emptyWidgets=slots.filter(v=>v.e==null),inventory={c:slots,a:{append(){}}};
 A.a=x=>x;
 A.l1=record=>{built.push({id:record.a,model:record.b,fake:record.c,used:record.d});return Object.assign(Object.create(A.aC.prototype),{c:record.a,e:record.b?$.p[record.b-1]:null,f:record.c?$.p[record.c-1]:null,w:record.d?{}:null,a:{style:{}},bH(){}});};
 let error=false;
 try{A.fH.prototype.lI.call(inventory,decoded);}catch(_){error=true;}finally{A.a=originalCast;A.l1=originalBuilder;}
 return {error,remaining_records:decoded.length,removed_slot_indices:removed,builder_inputs:built,empty_widget_identity_preserved:emptyWidgets.every(v=>inventory.c.includes(v)),slots:inventory.c.map(v=>({id:v.c,empty:v.e==null,used:v.w!=null,left:v.a.style.left,top:v.a.style.top}))};
}
function record(initial,extra=0){initial=initial.map(v=>({empty:v.empty,used:!v.empty&&v.used}));const count=initial.filter(v=>!v.empty).length;
 const rows=Array.from({length:count+extra},(_,i)=>({id:100+i,modelId:[118,235,6][i%3],fakeModelId:i%3===0?113:0,used:i%3===1}));
 const output=run(initial,rows);
 if(output.error||output.remaining_records!==extra||output.removed_slot_indices.length!==count||!output.empty_widget_identity_preserved||output.slots.length!==initial.length)throw new Error('Client redraw shape changed');
 let cursor=0;for(let i=0;i<initial.length;++i){if(initial[i].empty){if(!output.slots[i].empty||output.slots[i].id!==i+1)throw new Error('Placeholder changed');}else{const expected=rows[cursor++];if(output.slots[i].id!==expected.id||output.slots[i].used!==expected.used)throw new Error('Replacement record changed');}}
 cases.push({initial_slots:initial,replacement_records:rows,result:output});
}
for(let n=0;n<=18;++n)for(let pattern=0;pattern<5;++pattern)record(Array.from({length:n},(_,i)=>({empty:pattern===1||pattern===2&&i%2===0,used:pattern===3||pattern===4&&i%2===1})));
for(let k=0;k<128;++k){const n=random(19);record(Array.from({length:n},()=>({empty:random(3)===0,used:random(2)===1})),k%7===0?1:0);}
const short=run([{empty:false,used:false},{empty:true,used:false},{empty:false,used:true}],[{id:100,modelId:118}]);
if(!short.error)throw new Error('Short replacement array unexpectedly accepted');
const cleared=[];
for(let n=0;n<=18;++n){let removed=0;const pane={d:A.u(Array.from({length:n},()=>Object.assign(Object.create(A.aC.prototype),{l(){++removed;}})),t.k)};A.j9.prototype.jF.call(pane);if(pane.d.length!==0||removed!==n)throw new Error('Used pane clear changed');cleared.push({input_count:n,removed,remaining:pane.d.length});}
const report={schema_version:1,source:manifest.client_url,client_sha256:manifest.client_sha256,cards_sha256:manifest.cards_sha256,methods:['fH.lI (A.a DOM casts and A.l1 widget construction stubbed)','A.l0 item decoder','j9.jF used-pane clear'],inventory_helper_sha256:helperSHA,counts:{valid_inventory_cases:cases.length,short_inventory_cases:1,used_pane_cases:cleared.length},cases,short_input_result:short,used_pane_clear:cleared,network_or_application_startup:false,conclusions:['Every existing nonempty inventory slot is replaced, including used-miracle slots. Empty gift placeholders remain the same widgets.','Replacement physical IDs/model/fake/used values come from the supplied wire records; lI does not sample gifts, assign new IDs or force unused flags.','The replaceItems receiver clears the public used-model pane for its player; only that player\'s own physical hand uses lI. This report exercises the two helpers separately, not server event fanout.'],limitations:['Rendering replacement is not proof of server order relative to178 consumption, Dream application, incoming body, revival or gifts.','The v29 all-live-copies/new-token/unused/Dream-masked redraw transition is an explicit experimental contract beyond this receiver evidence.','A short supplied replacement list throws in the client helper; excess rows remain unused. The renderer does not establish an authoritative replacement-count validation rule.','DOM/widget visuals are stubbed, and no network, account, application startup or game connection runs.'],generator:'node scripts/official_redraw_oracle.cjs --client /path/to/main.dart.js [--check]'};
const output=JSON.stringify(report,null,2)+'\n';
if(check&&fs.readFileSync(path.join(directory,'redraw-client-v29.json'),'utf8')!==output)throw new Error('Redraw client report differs');
process.stdout.write(output);
