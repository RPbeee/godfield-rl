#!/usr/bin/env node
// Actual public-client numeric event updates, without startup, DOM or server.
'use strict';
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),crypto=require('node:crypto');
const dir=path.resolve(__dirname,'../data/official-v1'),manifest=JSON.parse(fs.readFileSync(path.join(dir,'manifest.json'),'utf8'));
const args=process.argv.slice(2);if(args.length!==2||args[0]!=='--client')throw new Error('Usage: --client /path/to/main.dart.js');
const bytes=fs.readFileSync(args[1]),digest=b=>crypto.createHash('sha256').update(b).digest('hex');if(digest(bytes)!==manifest.client_sha256)throw new Error('Pinned client SHA mismatch');
let source=bytes.toString('utf8'),marker='var s=A.wG';if(source.split(marker).length!==2)throw new Error('Export marker changed');source=source.replace(marker,'globalThis.oracle={A,B,J,t,$};var s=A.wG');
const box={dartMainRunner(){}};vm.createContext(box);vm.runInContext(source,box,{timeout:5000});const {A}=box.oracle;
const field=value=>({a:{textContent:String(value)},gK(){return this.a.textContent;}});
const methods=['eW','dH','bc'],lines=['# official-resource-vectors-v1: R initialHP initialMP initialCP resource(0HP1MP2CP) delta finalHP finalMP finalCP'];let count=0;
function record(hp,mp,cp,resource,delta){const p=Object.assign(Object.create(A.c5.prototype),{as:field(hp),at:field(mp),ax:field(cp)});p[methods[resource]](delta);
 const out=[p.as.gK(),p.at.gK(),p.ax.gK()].map(Number);if(out.some(x=>!Number.isSafeInteger(x)))throw new Error('Client produced unsafe final resource');
 lines.push(['R',hp,mp,cp,resource,delta,...out].join('\t'));++count;}
for(const hp of [0,1,2,40,98,99])for(const mp of [0,1,2,40,98,99])for(const cp of [0,1,2,40,98,99])for(let resource=0;resource<3;++resource)for(const delta of [-999,-100,-99,-1,0,1,99,100,999])record(hp,mp,cp,resource,delta);
let seed=0x45d9f3b;const random=n=>{seed^=seed<<13;seed^=seed>>>17;seed^=seed<<5;return(seed>>>0)%n;};
for(let i=0;i<2048;++i)record(random(100),random(100),random(100),random(3),random(2001)-1000);
for(let resource=0;resource<3;++resource)for(const initial of [[0,0,0],[1,1,1],[99,99,99]])for(const delta of [-Number.MAX_SAFE_INTEGER,Number.MAX_SAFE_INTEGER])record(...initial,resource,delta);
const output=lines.join('\n')+'\n';fs.writeFileSync(path.join(dir,'resource-vectors.tsv'),output);
const metadata={schema_version:1,source:manifest.client_url,client_sha256:manifest.client_sha256,vectors_sha256:digest(output),counts:{R:count},
 methods:['c5.eW','c5.dH','c5.bc'],network_or_application_startup:false,
 purpose:'Actual client HP/MP/CP delta and debt cascade only; does not prove server scheduling, gifts, death or acceptance.',
 generator:'node scripts/official_resource_oracle.cjs --client /path/to/main.dart.js'};
fs.writeFileSync(path.join(dir,'resource-vectors.manifest.json'),JSON.stringify(metadata,null,2)+'\n');console.log(JSON.stringify(metadata));
