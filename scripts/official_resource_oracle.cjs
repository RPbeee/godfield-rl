#!/usr/bin/env node
// Actual public-client numeric event updates, without startup, DOM or server.
'use strict';
const fs=require('node:fs'),path=require('node:path');
const {loadPinnedClient,writeOracleOutputs,digest}=require('./official_client_oracle.cjs');
const {A,binding,outputDirectory,manifest}=loadPinnedClient(process.argv.slice(2),'scripts/official_resource_oracle.cjs');
const field=value=>({a:{textContent:String(value)},gK(){return this.a.textContent;}});
const methods=binding.resources,lines=['# official-resource-vectors-v1: R initialHP initialMP initialCP resource(0HP1MP2CP) delta finalHP finalMP finalCP'];let count=0;
function record(hp,mp,cp,resource,delta){const p=Object.assign(Object.create(A.c5.prototype),{as:field(hp),at:field(mp),ax:field(cp)});p[methods[resource]](delta);
 const out=[p.as.gK(),p.at.gK(),p.ax.gK()].map(Number);if(out.some(x=>!Number.isSafeInteger(x)))throw new Error('Client produced unsafe final resource');
 lines.push(['R',hp,mp,cp,resource,delta,...out].join('\t'));++count;}
for(const hp of [0,1,2,40,98,99])for(const mp of [0,1,2,40,98,99])for(const cp of [0,1,2,40,98,99])for(let resource=0;resource<3;++resource)for(const delta of [-999,-100,-99,-1,0,1,99,100,999])record(hp,mp,cp,resource,delta);
let seed=0x45d9f3b;const random=n=>{seed^=seed<<13;seed^=seed>>>17;seed^=seed<<5;return(seed>>>0)%n;};
for(let i=0;i<2048;++i)record(random(100),random(100),random(100),random(3),random(2001)-1000);
for(let resource=0;resource<3;++resource)for(const initial of [[0,0,0],[1,1,1],[99,99,99]])for(const delta of [-Number.MAX_SAFE_INTEGER,Number.MAX_SAFE_INTEGER])record(...initial,resource,delta);
const output=lines.join('\n')+'\n';
const metadata={schema_version:1,source:manifest.client_url,client_sha256:manifest.client_sha256,vectors_sha256:digest(output),counts:{R:count},
 methods:methods.map(name=>'c5.'+name),network_or_application_startup:false,
 purpose:'Actual client HP/MP/CP delta and debt cascade only; does not prove server scheduling, gifts, death or acceptance.',
 generator:'node scripts/official_resource_oracle.cjs --client /path/to/main.dart.js'+(binding.version==='official-v1'?' --catalog-dir data/official-v1 --out-dir /tmp/official-v1-replay':'')};
writeOracleOutputs(outputDirectory,'resource',output,metadata);console.log(JSON.stringify(metadata));
