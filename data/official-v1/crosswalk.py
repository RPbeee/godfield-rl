"""Compare every legacy-v37 record with current official facts, conservatively."""
from pathlib import Path
import argparse, csv, hashlib, io, json
p=Path(__file__).resolve().parent
parser=argparse.ArgumentParser();parser.add_argument('--check',action='store_true');args=parser.parse_args()
rows=json.loads((p/'cards.json').read_text())
cols=['type','attack_kind','attack_extra','defense_kind','defense_extra','attribute','value','sub_value','hit_rate','price','weight']
old={i:dict(zip(cols,r))for i,r in enumerate(csv.reader((p.parent/'legacy-status-miracles-v1/immutable-item-data.csv').open()))}
used={}
for f in sorted(p.parent.glob('legacy-*/catalog.csv')):
 for r in csv.DictReader(f.open()):used[int(r['id'])]=f.parent.name
assert len(used)==231
category={'WEAPON':'weapons','PROTECTOR':'armor','MAGIC':'miracles','SUNDRY':'sundries','TRADE':'trade'}
element={'':'','FIRE':'fire','WATER':'water','TREE':'wood','SOIL':'stone','LIGHT':'light','DARK':'darkness'}
curse={'COLD':'cold','FEVER':'fever','HELL':'hell','HEAVEN':'heaven','FOG':'fog','ILLUSION':'dream','GLORY':'flash','DARK_CLOUD':'darkcloud'}
effects={'ABSORB_HP':'absorbHP','DAMAGE_TO_SELF':'dealSameDamage','MAGICAL':'atkBy2xMP','PESTLE':'danger','DYING_ATTACK':'attackDyingly','MAGIC_FREE':'cutCost','DOUBLE_ATK':'doubleAtk','WIDE_ATK':'attackEveryEnemy','ADD_ATTRIBUTE':'setElement','REVIVE':'revive','MORTAR':'attractDanger','FLICK_WEAPON':'bounceWeapon','REFLECT_WEAPON':'reflectWeapon','FLICK_MAGIC':'bounceMiracle','REFLECT_MAGIC':'reflectMiracle','BLOCK_MAGIC':'blockMiracle','BLOCK_WEAPON':'blockWeapon','REMOVE_ATTRIBUTE':'filterAtkElement','REFLECT_ANY':'reflectAnything','INCREASE_HP':'boostHP','INCREASE_MP':'boostMP','INCREASE_YEN':'boostCP','REMOVE_LOWER_HARMS':'removeMildCurses','REMOVE_ALL_HARMS':'removeAllCurses','SET_ASSISTANT':'setGuardian','REMOVE_ITEMS':'removeItems','REMOVE_ABILITIES':'removeUsedMiracles','INCREASE_OR_DECREASE_HP':'boostHPOrDealDamage','EXCHANGE':'exchange','SELL':'sell','BUY':'buy','MYSTERY':'callPhenomenon'}
fields=['category','element','atk','isPlusAtk','hitRate','def','ability','abilityValue','curse','cost','price','giftRate']
def legacy(id,r):
 v=int(r['value']or 0);sv=int(r['sub_value']or 0);kind=r['attack_kind'];extra=r['attack_extra'];de=r['defense_extra']
 d={'category':category[r['type']],'element':element[r['attribute']],'atk':v if kind=='ATK'else sv if extra=='INCREASE_ATK'and r['type']=='SUNDRY'else 0,'isPlusAtk':extra in ['INCREASE_ATK','ADD_ATTRIBUTE','DOUBLE_ATK','WIDE_ATK'],'hitRate':int(r['hit_rate']or 0),'def':(sv if r['type']=='WEAPON'else v)if r['defense_kind']=='DFS'else 0,'ability':'','abilityValue':0,'curse':'','cost':sv if r['type']=='MAGIC'else 0,'price':int(r['price']or 0),'giftRate':int(r['weight']or 0)}
 if kind in effects:d['ability']=effects[kind];d['abilityValue']=v if kind in ['INCREASE_HP','INCREASE_MP','INCREASE_YEN','INCREASE_OR_DECREASE_HP','REMOVE_ITEMS','REMOVE_ABILITIES']else 0
 if kind=='ADD_HARM':d['ability']='addCurse';d['curse']=curse[extra]
 if extra in effects:d['ability']=effects[extra];d['abilityValue']=v if extra in ['REVIVE','MORTAR']else 0
 if extra in curse and kind!='ADD_HARM':d['ability']='boostMPAndAddCurse'if kind=='INCREASE_MP'else'addCurseOnDamage';d['curse']=curse[extra]
 if de in effects:d['ability']=effects[de]
 elif de in curse:d['curse']=curse[de];d['ability']='selfCurse'
 if r['defense_kind']=='COUNTER':
  d['ability']={187:'counterAtk',190:'counter2xAtk',191:'counterCurse',192:'counterCurse',193:'counterBoost2xMP',194:'counterTakeCP'}[id]
 if extra=='DYING_ATTACK':d['abilityValue']=sv
 if extra=='WIDE_ATK':d['hitRate']=0 # The old forced100 WIDE rollout is an effect, not the current base hitRate display.
 return d

def current(r):return {k:r.get(k,False if k=='isPlusAtk'else ''if k in ['element','ability','curse']else 0)for k in fields}
records=[]
for id in sorted(used):
 d=legacy(id,old[id]);pool=[r for r in rows if r['category']==d['category']]
 # Structural evidence is candidate-level only. Never infer names or identity from numeric positions.
 same_role=[r for r in pool if all(current(r)[k]==d[k]for k in ['element','ability','curse','isPlusAtk'])]
 same_power=[r for r in same_role if all(current(r)[k]==d[k]for k in ['atk','def','hitRate','abilityValue'])]
 exact=[r for r in same_power if all(current(r)[k]==d[k]for k in ['cost','price','giftRate'])]
 candidates=exact or same_power or same_role
 level='equal_comparable_metadata'if exact else'role_and_power'if same_power else'role_only'if same_role else'no_candidate'
 status='provisional_unique'if len(candidates)==1 else'ambiguous'if candidates else'unmapped'
 entry={'legacy_id':id,'legacy_catalog':used[id],'legacy_source_row':old[id],'projected_legacy_facts':d,'mapping_status':status,'identity_verified':False,'candidate_basis':level,'official_candidates':[]}
 for r in candidates:
  c=current(r);entry['official_candidates'].append({'official_id':r['id'],'image_name':r['imageName'],'name':r['name'],'field_differences':{k:{'legacy':d[k],'official':c[k]}for k in fields if d[k]!=c[k]}})
 records.append(entry)
summary={k:sum(r['mapping_status']==k for r in records)for k in ['provisional_unique','ambiguous','unmapped']}
result={'schema_version':1,'purpose':'Research crosswalk, never a runtime ID conversion. All candidates remain identity-unverified because the archived legacy CSV has no names/image slugs.','official_cards_sha256':hashlib.sha256((p/'cards.json').read_bytes()).hexdigest(),'legacy_supported_count':231,'summary':summary,'glossary_caveats':['TREE->wood/SOIL->stone/DARK->darkness are label projections. GLORY->flash and ILLUSION->dream are tentative comparison vocabulary, not proof of effect equivalence.','Legacy subvalue/INCREASE_ATK/ADD_ATTRIBUTE and self-cursetag roles are projected for comparison, not asserted operationally equal.',
'An absent official abilityValue=0 does not mean a zero effect. Current removeItems count3 and other handler constants are not catalog fields; do not turn metadata differences into behavioral conclusions.','role_only candidates can differ in every power/value/cost/weight. Unique candidate does not prove historical identity.','No positional/numeric-ID matching, name invention or unsupported effect removal. Unknown identity is retained explicitly.'],'records':records}
text=json.dumps(result,ensure_ascii=False,indent=2)+'\n';path=p/'legacy-v37-crosswalk.json'
if args.check:assert path.read_text()==text,'crosswalk needs regeneration'
else:path.write_text(text)
print('Crosswalk231:',summary)
