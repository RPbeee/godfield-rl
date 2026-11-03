#!/usr/bin/env node
// Independent LOCAL v47 random-stage reference, not a guardian scheduler emulator.
// Existing primary evidence is inherited by exact hash; no new selector is found.
'use strict';
const fs=require('node:fs'),path=require('node:path');
const {loadPinnedClient,digest}=require('./official_client_oracle.cjs');
let client=null,check=false;const args=process.argv.slice(2);
for(let i=0;i<args.length;++i){if(args[i]==='--check'&&!check)check=true;else if(args[i]==='--client'&&client===null&&i+1<args.length&&!args[i+1].startsWith('--'))client=args[++i];else throw new Error('Usage: node scripts/official_guardian_payload_auto_oracle.cjs --client PATH [--check]');}
if(client===null)throw new Error('--client is required');
const directory=path.resolve(__dirname,'../data/official-v2');
const {binding,manifest}=loadPinnedClient(['--client',client,'--catalog-dir',directory],'scripts/official_guardian_payload_auto_oracle.cjs');
if(binding.version!=='official-v2')throw new Error('Expected pinned official-v2');
const raw=fs.readFileSync(path.join(directory,'guardian-payload-sampler-client-v46.json'));
const inheritedSHA='9213e59fb379d621a3a442428119b00ca2c23ddc162eed0e01f0b909bdce35bb';
if(digest(raw)!==inheritedSHA)throw new Error('Archived v46 selector reference changed');
const previous=JSON.parse(raw),facts=JSON.parse(fs.readFileSync(path.join(directory,'cards.json'),'utf8'));
if(previous.client_sha256!==manifest.client_sha256||previous.cards_sha256!==manifest.cards_sha256)throw new Error('Inherited source tuple differs');
const domains=previous.official_evidence.append;
const MOD=1n<<64n,MASK=MOD-1n,GAMMA=0x9e3779b97f4a7c15n,M1=0xbf58476d1ce4e5b9n,M2=0x94d049bb133111ebn;
const u64=x=>x&MASK,hex=x=>'0x'+u64(x).toString(16).padStart(16,'0');
function word(state){state=u64(state+GAMMA);let z=state;z=u64((z^(z>>30n))*M1);z=u64((z^(z>>27n))*M2);return{state,value:u64(z^(z>>31n))};}
function bounded(rng,n,stage){const b=BigInt(n),threshold=MOD%b,rejected=[];let next;do{next=word(rng.state);rng.state=next.state;if(next.value<threshold)rejected.push(hex(next.value));}while(next.value<threshold);const result=Number(next.value%b);if(rng.calls)rng.calls.push({stage,bound:n,threshold:hex(threshold),rejected_words:rejected,accepted_word:hex(next.value),result});return result;}
function inverse(a){let oldR=MOD,r=a,oldT=0n,t=1n;while(r){const q=oldR/r;[oldR,r]=[r,oldR-q*r];[oldT,t]=[t,oldT-q*t];}return u64(oldT);}
function undoRight(y,n){let x=0n;for(let shift=0n;shift<64n;shift+=n)x^=y>>shift;return u64(x);}
function seedForFirst(output){let x=undoRight(output,31n);x=u64(x*inverse(M2));x=undoRight(x,27n);x=u64(x*inverse(M1));x=undoRight(x,30n);const seed=u64(x-GAMMA);if(word(seed).value!==output)throw new Error('Independent inverse failed');return seed;}
function person(id,team,guardian=null,mp=0){return{id,team,dead:false,hp:99,mp,guardian};}
const fixtures={earth:{owner:7,owner_command:'pray',owner_has_disease:false,players:[person(7,2),person(42,1,'earth')]},moon:{owner:7,owner_command:'pray',owner_has_disease:false,players:[person(7,2),person(0,1,'moon')]},earth_moon:{owner:7,owner_command:'pray',owner_has_disease:false,players:[person(7,2),person(42,1,'earth'),person(0,1,'moon')]},two_moons:{owner:7,owner_command:'pray',owner_has_disease:false,players:[person(7,2),person(42,1,'moon'),person(0,1,'moon')]}};
fixtures.earth_mp17=structuredClone(fixtures.earth);fixtures.earth_mp17.players[1].mp=17;
fixtures.earth_mp99=structuredClone(fixtures.earth);fixtures.earth_mp99.players[1].mp=99;
const enemy=(a,b)=>(a.team==='solo'&&a.id!==b.id)||a.team!==b.team;
function defaultEnemy(id){const c=facts[id-1];if(c.hitRate>0)return false;return(c.atk||0)>0||c.category==='weapons'||['sell','buy','boostCPToEnemy','takeCP','addCurse','removeItems','removeUsedMiracles'].includes(c.ability);}
const power=models=>models.reduce((sum,id)=>sum+(facts[id-1].atk||0),0);
function select(players,source,rng){const guardian=source.guardian,baseIDs=domains[guardian].base_ids,models=[baseIDs[bounded(rng,baseIDs.length,source.id+':base')]];
    while(models.length<18){const candidates=domains[guardian].by_base[models[0]];if(!candidates.length||bounded(rng,2,source.id+':continue'+models.length)===0)break;models.push(candidates[bounded(rng,candidates.length,source.id+':extra'+models.length)]);}
    const hostile=power(models)>0||defaultEnemy(models[0]);const targets=players.filter(p=>!p.dead&&enemy(source,p)===hostile).map(p=>p.id);
    return{source:source.id,guardian,models,target:targets[bounded(rng,targets.length,source.id+':target')],target_candidates:targets};
}
// Only the explicitly small no-death self primitives are allowed to continue
// to another source. Every other body stops before response/effect execution.
function run(seed,fixtureName='earth',trace=true){const ctx=fixtures[fixtureName],players=structuredClone(ctx.players),rng={state:seed,calls:trace?[]:null},records=[];let stop='all-opportunities-finished-before-gifts';
    for(const source of players){if(source.dead||!source.guardian||!enemy(source,players.find(p=>p.id===ctx.owner)))continue;
        if(!['earth','moon'].includes(source.guardian))throw new Error('Reference excludes other eight canonical guardians');
        const record={source:source.id,guardian:source.guardian,opportunity_ticket:bounded(rng,100,source.id+':opportunity')};records.push(record);
        if(record.opportunity_ticket>=25){record.result='quarter-miss';continue;}
        const action=select(players,source,rng),base=facts[action.models[0]-1];record.action=action;
        record.repeat_frames=base.ability==='attackTwice'?2:1;
        if(base.ability==='boostHPOrDealDamage')record.random_hp_delta=bounded(rng,2,source.id+':202-sign')===0?10:-10;
        if(base.ability==='atkBy2xMP'){record.launch_mp=source.mp;record.launch_power=2*source.mp+power(action.models);source.mp=0;record.source_mp_after_launch=0;}
        if((base.hitRate||0)>0){record.result='stop-before-chance-activation';stop=record.result;break;}
        if((base.atk||0)>0||base.category==='weapons'||action.target!==source.id){record.result='first-response-not-executed';stop=record.result;break;}
        if([201,239].includes(base.id)){const code=1+bounded(rng,10,source.id+':self-summon-type');record.summoned_guardian_code=code;source.guardian=manifest.enums.Guardian[code];record.result='small-self-summon-body-finished';}
        else if(base.id===202){const before=source.hp;source.hp=Math.max(0,Math.min(99,before+record.random_hp_delta));record.hp_change=source.hp-before;
            if(record.hp_change<0&&source.guardian){const ticket=bounded(rng,100,source.id+':self202-departure');record.departure_ticket=ticket;record.departed=ticket<10;if(record.departed)source.guardian=null;}
            record.result='small-self202-body-finished';
        }else if(base.id===235){const before=source.hp;source.hp=Math.min(99,source.hp+10);record.hp_change=source.hp-before;record.result='small-self235-body-finished';}
        else{record.result='stop-before-unmodelled-body';stop=record.result;break;}
    }
    return{seed:hex(seed),fixture:fixtureName,records,next_rng:hex(rng.state),calls:rng.calls,stop,post_reference_players:players};
}
function find(fixture,test){for(let seed=0n;seed<3000000n;++seed){const value=run(seed,fixture,false);if(test(value))return run(seed,fixture);}throw new Error('Finite stage fixture search failed: '+fixture);}
const vectors=[],add=(label,value)=>vectors.push({label,...value});
add('earth-quarter-miss',run(0n,'earth'));add('moon-quarter-miss',run(0n,'moon'));
add('earth-quarter24-accepted',run(seedForFirst(124n),'earth'));add('earth-quarter25-rejected',run(seedForFirst(125n),'earth'));
add('quarter-bound100-rejects-first-zero',run(u64(-GAMMA),'earth'));
add('earth14-two-fresh-frames-one-launch',find('earth',v=>v.records[0]?.action?.models[0]===14));
add('earth14-compound-two-fresh-frames',find('earth',v=>v.records[0]?.action?.models[0]===14&&v.records[0].action.models.length>=3));
add('earth58-compound-sourceMP17-launch',find('earth_mp17',v=>v.records[0]?.action?.models[0]===58&&v.records[0].action.models.length>=3));
const mana=find('earth',v=>v.records[0]?.action?.models[0]===58);add('earth58-zero-sourceMP-at-launch',mana);add('earth58-sourceMP17-same-selector',run(BigInt(mana.seed),'earth_mp17'));add('earth58-sourceMP99-same-selector',run(BigInt(mana.seed),'earth_mp99'));
add('earth202-positive-self-cap99',find('earth',v=>v.records[0]?.action?.models[0]===202&&v.records[0].random_hp_delta===10));
add('earth202-negative-self-no-departure',find('earth',v=>v.records[0]?.action?.models[0]===202&&v.records[0].random_hp_delta===-10&&!v.records[0].departed));
add('earth202-negative-self-departure',find('earth',v=>v.records[0]?.action?.models[0]===202&&v.records[0].random_hp_delta===-10&&v.records[0].departed));
add('moon235-self-zeroMP-cap99',find('moon',v=>v.records[0]?.action?.models[0]===235));
add('earth201-self-type-draw',find('earth',v=>v.records[0]?.action?.models[0]===201));add('moon239-self-type-draw',find('moon',v=>v.records[0]?.action?.models[0]===239));
add('two-immediate-self-summons-earth201-moon239',find('earth_moon',v=>v.records.length===2&&v.records[0]?.action?.models[0]===201&&v.records[0].action.target===42&&v.records[1]?.action?.models[0]===239&&v.records[1].action.target===0));
add('two-immediate-self-summons-moon239-twice',find('two_moons',v=>v.records.length===2&&v.records[0]?.action?.models[0]===239&&v.records[0].action.target===42&&v.records[1]?.action?.models[0]===239&&v.records[1].action.target===0));
add('first-source-miss-second-moon235-self',find('earth_moon',v=>v.records.length===2&&v.records[0].result==='quarter-miss'&&v.records[1]?.action?.models[0]===235&&v.records[1].action.target===0));
add('first-response-doesnot-preselect-next-source',find('earth_moon',v=>v.records.length===1&&v.records[0]?.action?.models[0]===14));
if(vectors.length!==21)throw new Error('Reference coverage changed');
const zero=vectors.find(v=>v.label==='quarter-bound100-rejects-first-zero');if(zero.calls[0].rejected_words[0]!==hex(0n)||zero.calls[0].threshold!==hex(16n))throw new Error('Quarter zero rejection changed');
const report={schema_version:1,title:'Independent LOCAL automatic Earth/Moon payload random-stage reference v47',source:manifest.client_url,client_sha256:manifest.client_sha256,cards_sha256:manifest.cards_sha256,primary_evidence:{new_primary_receiver_executions:0,inherited:{path:'data/official-v2/guardian-payload-sampler-client-v46.json',sha256:inheritedSHA,bytes:raw.length},earth_moon_category_notes:previous.official_evidence.locales,base_domains:{earth:120,moon:26},boundary:'The v46 source append-domain observations and its narrow LOCAL selector are inherited unchanged. This adds no official automatic selector,MP policy,target law or timing evidence. The published25percent opportunity wording doesnot establish draw staging or an Earth/Moon server algorithm.'},local_reference:{official_client_selector:false,constants:{gamma:hex(GAMMA),multiply1:hex(M1),multiply2:hex(M2)},bounded_rule:'Unsigned SplitMix64;accept output>=2^64modn then outputmodn. All words/states use exact hexadecimal uint64.',opportunity_rule:'A normal owner7 pray with no disease makes no game draw before the first guardian stage. Alive enemy source order is input roster order. One bounded100<25 opportunity per present source. On success immediately call the v46 uniform120/26 column selector using the SAME game stream;do not consume the old285/286 singleton move draw.',payload_rule:previous.local_reference.base_rule+' '+previous.local_reference.extra_rule+' '+previous.local_reference.target_rule,launch_rule:'After selection,202 draws one bounded2 sign;58 captures sourceMP and drains it to0 once,without another draw.14 records two fresh frames from the same immutable selected column;the reference stops at its first response. Percentage activation is a later draw and is not performed here.',continued_bodies:'Only immediate self201/239 draw one bounded10+1 guardian code;only self202 clipsHP99 by its signed10 and makes the existing local10percent departure draw after positive actual loss;only self235 clipsHP+10. No death is possible in these HP99 fixtures. Any other body,foreign target or response stops this reference before execution.',ownership:'These are game-stream stage examples,not calls to the const external chance helper and not a complete Match executor. One ordinary decision can contain two self summon bodies. Main owner/gift debt/decision counters are not recreated;no source inventory/token/fee/retention is added.',fixtures,vectors},counts:{local_stage_vectors:vectors.length,new_primary_receiver_executions:0,largest_column:Math.max(...vectors.flatMap(v=>v.records.map(r=>r.action?.models.length||0))),multi_source_summon_vectors:2},limitations:['This report is a small independent LOCAL random-stage reference,not an official guardian scheduler golden oracle. Uniform base/extra/target laws and same-stream draw ordering are explicit engine policy.','Old default canonical Earth285/Moon286 automatic behavior is unchanged when the new opt-in is false. The two first-eight guardian distributions and their existing behavior are outside this reference.','No owner tick,disease,body damage,response,reflection,bounce,counter,posthumous112,revival208,Fog projection,grant phase,gift delivery,GFadvance,natural end,cutoff,proof replay or transaction machinery is emulated. A next_rng here is only the named stopping boundary,not necessarily the final RNG after a whole Match step.','The two-source summon examples intentionally choose self for both sources. Earlier summoning therefore cannot change the still-unprocessed next source. Each source is visited once even if its own type changes. These fixtures demonstrate multiple local summons per decision,not server behavior.','The HP99 self202 examples include only the pre-existing local direct-loss guardian departure draw;they cannot die or trigger208/112. Other body or prefix side effects are deliberately not synthesized.','A first-response record contains no sampled future guardian payload. Next sources must wait for the real response/counter/death work and use the resulting live state. This reference is privileged fixture data,not a public observation.','No network/account/Dart startup or C++engine/build runs. No existing source,report,pin,docs or core implementation is changed.'],network_or_application_startup:false,generator:'node scripts/official_guardian_payload_auto_oracle.cjs --client /path/main.dart.js [--check]'};
const output=JSON.stringify(report,null,2)+'\n';if(check&&fs.readFileSync(path.join(directory,'guardian-payload-auto-local-v47.json'),'utf8')!==output)throw new Error('Automatic guardian payload local reference differs');process.stdout.write(output);
