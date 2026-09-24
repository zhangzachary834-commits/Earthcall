#!/usr/bin/env python3
"""Add Astra's Court of the Open Hand without regenerating anyone else's world.

Zach commissioned an original, spatially separate Cathedral addition. Composition
and implementation: Codex / GPT-6 Astra, session
01a07eb3-8ee7-7aa3-8b34-65fea2f4cd44, 2026-09-19T12:21:43-07:00.
See docs/Agenda/Tasks/Specific Tasks/Cathedral_Open_Hand/Cathedral_Open_Hand.md.
No import of generate_cathedral: that module executes a destructive regeneration.
"""
import argparse
from copy import deepcopy
import hashlib
import json
import math
from pathlib import Path
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ZONE = 'Cathedral of the Living Logos'
PREFIX = 'cathedral.astra.openhand.'
MAT = 'material.astra.openhand.'
LAW = 'law-astra-openhand-'
SESSION = '01a07eb3-8ee7-7aa3-8b34-65fea2f4cd44'
STAMP = '2026-09-19T12:21:43-07:00'
CX, CZ = 44.0, 10.0


def pv(v):
    return {'t': 'bool' if isinstance(v, bool) else 'string' if isinstance(v, str) else 'double', 'v': v}


def matrix(pos, angles=(0, 0, 0)):
    x, y, z = map(math.radians, angles)
    sx, cx, sy, cy, sz, cz = math.sin(x), math.cos(x), math.sin(y), math.cos(y), math.sin(z), math.cos(z)
    return [cy*cz, cy*sz, -sy, 0, cz*sx*sy-cx*sz, cx*cz+sx*sy*sz, cy*sx, 0,
            sx*sz+cx*cz*sy, cx*sy*sz-cz*sx, cx*cy, 0, *pos, 1]


def leaf(prim, dims, off=(0, 0, 0), rounding=0):
    node = {'op': 0, 'prim': prim, 'dims': list(dims), 'offset': list(off),
            'p0': rounding, 'p1': 0, 't': .5, 'children': []}
    if prim == 0:
        # Author the distance expression explicitly. This selects the existing
        # gradient-aware marcher for the thin, concave spherical leaf cavities.
        node['prim']=7
        node['expr']=f'sqrt(x*x+y*y+z*z)-{dims[0]:.9g}'
    return node


def csg(op, a, b):
    return {'op': op, 'prim': 0, 'dims': [.5]*3, 'offset': [0]*3,
            'p0': 0, 'p1': 0, 't': .1, 'children': [a, b]}


def term(c, factors=None, trans=None):
    t = {'c': c, 'factors': factors or {}}
    if trans:
        t['trans'] = trans
    return t


def wave(var, scale=1, shift=0, kind=0):
    return {'kind': kind, 'var': var, 'scale': scale, 'shift': shift}


def map_action(path, bindings, terms):
    return {'kind': 8, 'path': path, 'bindings': bindings,
            'function': {'pieces': [{'expr': {'terms': terms}}]}}


def relation(a, b, kind):
    return {'entityA': a, 'entityB': b, 'type': kind, 'directed': True, 'weight': 1.0, 'events': []}


def build():
    objects, materials, laws, relations = [], [], [], []
    commission = 'lexeme.astra.openhand.commission'
    lexemes = [{'id':commission,'symbol':'Zach commissioned Astra to make a separate Court of the Open Hand in the Cathedral, 2026-09-19.'}]
    material_templates = {}
    palette = {
        'ivory': (.78, .72, .59), 'basalt': (.055, .09, .12),
        'bronze': (.70, .39, .13), 'gold': (.98, .70, .24),
        'verdigris': (.10, .42, .39), 'seed': (.46, .68, .84),
        'pearl': (.83, .88, .84), 'lapis': (.07, .16, .32),
    }
    for name, rgb in palette.items():
        m = {'name': 'astra.openhand.'+name, 'baseColor': list(rgb), 'opacity': 1.0,
             'ambient': .35, 'diffuse': .8, 'specular': .65, 'shininess': 70.0,
             'textureResolution': 16}
        if name in ('seed', 'bronze', 'verdigris', 'ivory'):
            # Bounded pigment, evaluated in local field coordinates. No fake
            # shadows, unimplemented emission, or claims of infinite precision.
            amplitude = .10 if name == 'seed' else .035
            channels = []
            for i, base in enumerate(rgb):
                channels.append({'op': 0, 'scalarForm': {'terms': [term(base),
                    term(amplitude, trans=[wave('y', 1.8 if name == 'seed' else 2.4, i*.8),
                                           wave('x', 1.2, kind=1)])]}})
            m['colorExpr'] = {'input': 'y', 'pieces': [{'mathNode': {'op': 2, 'children': channels}}]}
        material_templates[name] = m

    def obj(slug, title, pos, tree, extent, material, rotation=(0, 0, 0), props=None, scale=(1,1,1)):
        ident = PREFIX+slug
        # Own the surface from birth. The current loader calls the texture-
        # resolution setter on reload, which otherwise forks shared Materials.
        mat = deepcopy(material_templates[material]);mat['name']=ident;materials.append(mat)
        relations.append(relation('material.'+ident,commission,'commissioned-under'))
        # A nonuniformly transformed sphere is an ellipsoid with conservative
        # local distance steps. The approximate ellipsoid SDF can overshoot on
        # slender forms; use the exact sphere in its own local coordinates.
        if tree['op']==0 and tree['prim']==3:
            dims=tree['dims'];scale=tuple(scale[i]*dims[i] for i in range(3))
            extent=tuple(extent[i]/dims[i] for i in range(3));tree=leaf(0,(1,1,1))
        transform=matrix(pos,rotation)
        for c in range(3):
            for r in range(3):transform[c*4+r]*=scale[c]
        properties = {'displayName': pv(title), 'injectedBy': pv('Codex / GPT-6 Astra'),
                      'authorizingPerson': pv('Zach'), 'authoringSession': pv(SESSION),
                      'authoredAt': pv(STAMP), 'openHandMember': pv(True)}
        properties.update({k: pv(v) for k, v in (props or {}).items()})
        o = {'objectID': ident, 'shapeKind': 10, 'geometryType': 10,
             'shapeParams': [.5, .5, .5, .5, 0, 0, 0, 0, 0, 0, 0],
             'field': tree, 'fieldExtent': list(extent), 'transform': transform,
             'center': list(pos), 'materialId': 'material.'+ident, 'renderMode': 0,
             'faceColors': [list(palette[material]) for _ in range(6)],
             'authoredProperties': properties}
        objects.append(o)
        relations.append(relation(ident, commission, 'commissioned-under'))
        if slug != 'touchstone':
            relations.append(relation(ident, PREFIX+'touchstone', 'composes-open-hand-court'))
        return o

    def ring(slug, title, pos, radius, thickness, material, rotation=(90, 0, 0)):
        return obj(slug, title, pos, leaf(6, (radius, thickness, 0)),
                   (radius+thickness+.03, radius+thickness+.03, thickness+.03), material, rotation)

    # Flat, walkable terraces: cylinder's authored axis is Z, rotated into Y.
    for i, (radius, y, halfheight, material) in enumerate([
            (12, .03, .18, 'basalt'), (11.5, .24, .10, 'bronze'),
            (11.2, .42, .10, 'ivory'), (6.8, .57, .055, 'lapis')]):
        obj(f'terrace.{i}', 'Open Hand / concentric terrace '+str(i+1), (CX, y, CZ),
            leaf(4, (radius, halfheight, 0)), (radius+.05, radius+.05, halfheight+.03), material, (90, 0, 0))
    for i, radius in enumerate((6.55, 7.0, 10.8)):
        ring(f'inlay.{i}', 'A real bronze line set into the court', (CX, .54, CZ), radius, .025, 'gold')
    # A compass rose has actual raised edges; nothing depicts a carved relief.
    for i in range(24):
        a = 2*math.pi*i/24
        r = 7.75
        obj(f'compass.{i:02}', 'Compass of welcome / ray '+str(i+1),
            (CX+r*math.sin(a), .553, CZ+r*math.cos(a)), leaf(2, (.025, .016, .65 if i%2 else 1.05), rounding=.01),
            (.05, .04, 1.1), 'gold' if i%2 == 0 else 'verdigris', (0, math.degrees(a), 0))

    # Seven open arches form the rear crescent. Their voids are CSG subtraction.
    # Positive local Z faces the visitor; the lower half is clipped at the floor.
    outer = csg(3, leaf(4, (4.0, .24, 0), (-2.35, 0, 0)), leaf(4, (4.0, .24, 0), (2.35, 0, 0)))
    inner = csg(3, leaf(4, (3.70, .65, 0), (-2.35, 0, 0)), leaf(4, (3.70, .65, 0), (2.35, 0, 0)))
    arch = csg(3, csg(4, outer, inner), leaf(1, (5, 6/1.8, 2), (0, 2.5/1.8, 0)))
    for i in range(7):
        angle = 100 + i*(160/6)
        a = math.radians(angle)
        x, z = CX+9.4*math.sin(a), CZ+9.4*math.cos(a)
        obj(f'portal.{i}', 'Seven invitations / open lancet '+str(i+1), (x, 4.03, z), arch,
            (1.75, 6/1.8, .3), 'ivory', (0, angle, 0),scale=(1,1.8,1))
        obj(f'portal.foot.{i}', 'Foot of invitation '+str(i+1), (x, .70, z),
            leaf(2, (1.9, .17, .50), rounding=.06), (2,.25,.6), 'basalt', (0,angle,0))
        # Small suspended tear inside each arch: both void and jewel remain real.
        obj(f'portal.tear.{i}', 'Verdigris tear '+str(i+1), (x, 7.3, z),
            leaf(3, (.18, .55, .18)), (.22,.6,.22), 'verdigris')

    # The heart is a twelve-leaved vessel with a reachable causal state.
    shell = csg(4, leaf(0, (1,1,1)), leaf(0, (.86,.86,.86), (0,.06,.40)))
    for i in range(12):
        angle = i*30
        a = math.radians(angle)
        s, c = math.sin(a), math.cos(a)
        obj(f'leaf.{i:02}', 'Open Hand / bronze leaf '+str(i+1),
            (CX+2.45*s, 4.5, CZ+2.45*c), shell, (1.06,1.06,1.06),
            'bronze' if i%2 == 0 else 'verdigris', (0,angle,0),
            {'openHandLeaf': True, 'radialX': s, 'radialZ': c, 'azimuth': angle},scale=(.78,3.35,1.05))
    obj('heart.plinth', 'Quiet plinth beneath the seed', (CX,.88,CZ),
        leaf(4, (1.6,.25,0)), (1.65,1.65,.30), 'ivory',(90,0,0))
    ring('heart.rim', 'Golden rim of the quiet center', (CX,1.14,CZ),1.43,.065,'gold')
    obj('seed', 'The seed / intention held in the open', (CX,4.7,CZ),
        leaf(3, (.88,1.35,.88)), (.95,1.42,.95), 'seed', props={'openHandSeed':True})
    # Three slender meridians around the seed, with honest openings between them.
    for i, rot in enumerate(((0,0,0),(0,60,0),(0,120,0))):
        ring(f'seed.meridian.{i}', 'Meridian around the seed '+str(i+1), (CX,4.7,CZ),1.65,.036,'gold',rot)
    # A crown reads as a distant landmark while preserving an open sky overhead.
    ring('crown', 'Open crown above the hand', (CX,12.2,CZ),3.25,.085,'bronze')
    for i in range(12):
        a = i*math.pi/6
        pos = (CX+3.25*math.sin(a),11.7,CZ+3.25*math.cos(a))
        obj(f'crown.drop.{i}', 'Crown pendant '+str(i+1),pos,leaf(3,(.11,.45,.11)),(.15,.5,.15),'pearl')
    # Two facing stone seats leave the approach open and invite staying.
    for side in (-1,1):
        x, z = CX+side*8.3, CZ+3.6
        obj(f'seat.{side}', 'A place to stay / '+('left' if side<0 else 'right'),(x,1.02,z),
            leaf(2,(.6,.18,1.7),rounding=.12),(.75,.32,1.85),'ivory')
        for j in (-1,1):
            obj(f'seat.foot.{side}.{j}', 'Stone seat support',(x,.72,z+j*1.15),
                leaf(2,(.42,.22,.25),rounding=.05),(.5,.3,.35),'basalt')
    # Entry has no global HUD. A pearl touchstone is physically reachable.
    obj('touchstone.base','Pedestal of the opening gesture',(CX,.92,CZ+9.35),
        leaf(3,(.52,.4,.52)),(.58,.45,.58),'bronze')
    obj('touchstone','THE OPEN HAND — click to open; click again to gather', (CX,1.47,CZ+9.35),
        leaf(3,(.40,.29,.40)),(.46,.35,.46),'pearl',props={
            'openHandTouchstone':True, 'aperture':0.0, 'intention':0.0,
            'description':'Click this pearl to open the twelve leaves. Click again to gather them. Both movements belong to authored Laws.',
            'origin':'Zach: make something original without overlapping existing work; Astra: Court of the Open Hand composition.'})
    for i in range(3):
        obj(f'approach.{i}', 'Threshold step '+str(i+1), (CX,.12+i*.12,CZ+13.0-i*.7),
            leaf(2,(2.4,.08,.42),rounding=.06),(2.5,.15,.5),'ivory')

    def law(slug, title, condition, actions, event=False):
        ident = LAW+slug
        l = {'id':ident,'name':'Open Hand: '+title,'enabled':True,'authority':0,
             'activation':0 if event else 1,'scope':0 if event else 1,'drives':False,'retrigger':0,
             'conditionMode':'all','authors':['Zach'],'conditionSubjects':[],'targets':[],
             'applicationLog':[],'conditionModel':condition,'actionModel':{'kind':5,'children':actions},
             'provenance':[relation(ident,commission,'commissioned-under')]}
        laws.append({'identifier':ident,'injected_by':{'agent':'Codex / GPT-6 Astra','session':SESSION,'timestamp':STAMP},
                     'authors':['Zach'],'law':l,'triggers':['object-clicked'] if event else []})
    def tagged(prop):
        return {'kind':0,'path':prop,'op':0,'operand':pv(True)}
    law('gesture','the hand opens and gathers',tagged('openHandTouchstone'),
        [map_action('intention',{'q':'intention'},[term(1),term(-1,{'q':1})])],True)
    # Exact exponential approach per elapsed frame; a long frame cannot overshoot.
    exp = [wave('d',-2.5,kind=2)]
    law('approach','intention becomes form without a jump',tagged('openHandTouchstone'),
        [map_action('aperture',{'a':'aperture','q':'intention','d':'time.delta'},
                    [term(1,{'q':1}),term(1,{'a':1},exp),term(-1,{'q':1},exp)])])
    binds = {'a':'@'+PREFIX+'touchstone.aperture','s':'radialX','c':'radialZ','r':'azimuth'}
    # Author the complete rotation: editing only Euler X would inherit an
    # equivalent-but-different Euler decomposition on the far half of the ring.
    rotation = {'kind':8,'path':'rotation','bindings':binds,'function':{'pieces':[
        {'mathNode':{'op':2,'children':[{'op':0,'scalarForm':{'terms':terms}} for terms in
            ([term(34,{'a':1})],[term(1,{'r':1})],[term(0)])]}}]}}
    law('unfold','twelve leaves follow one continuous aperture',tagged('openHandLeaf'),[
        map_action('position.x',binds,[term(CX),term(2.45,{'s':1}),term(2.35,{'s':1,'a':1})]),
        map_action('position.z',binds,[term(CZ),term(2.45,{'c':1}),term(2.35,{'c':1,'a':1})]),
        map_action('position.y',binds,[term(4.5),term(-.35,{'a':1})]),
        rotation])
    return {'objects':objects,'materials':materials,'relations':relations,'laws':laws,'lexemes':lexemes}


def object_bounds(o):
    """Conservative authored box, including rotation; 2D overlays have no site."""
    if o.get('shapeKind') in (12,13):
        return None
    t=o['transform']
    if 'fieldExtent' in o:
        e=o['fieldExtent']
    elif o.get('shapeKind',0)==0:
        e=[.5]*3
    else:
        # Cathedral primitives use radius parameters; this intentionally
        # overestimates their volume when checking the reserved addition site.
        e=[max([.5]+[abs(float(v)) for v in o.get('shapeParams',[])])]*3
    h=[sum(abs(t[4*j+i])*e[j] for j in range(3)) for i in range(3)]
    return [[t[12+i]-h[i],t[12+i]+h[i]] for i in range(3)]


def append_zone(z, package):
    old=deepcopy(z)
    if any(o['objectID'].startswith(PREFIX) for o in z['world']['objects']):
        raise ValueError('Court already exists. Refusing to replace authored work.')
    # Covers the moving leaves in either state as well as every static Object.
    site=((31.5,56.5),(-.3,13),(0,24))
    for o in z['world']['objects']:
        b=object_bounds(o)
        if b and all(b[i][1]>=site[i][0] and b[i][0]<=site[i][1] for i in range(3)):
            raise ValueError('Reserved site intersects existing '+o['objectID'])
    for m in z.get('materials',[]):
        if m['name'].startswith(PREFIX):
            raise ValueError('Existing Court material: '+m['name'])
    z['world']['objects'].extend(deepcopy(package['objects']))
    z.setdefault('materials',[]).extend(deepcopy(package['materials']))
    z.setdefault('formationRelations',[]).extend(deepcopy(package['relations']))
    z.setdefault('lexemes',[]).extend(deepcopy(package['lexemes']))
    z.setdefault('lawRefs',[]).extend(l['identifier'] for l in package['laws'])
    # Remove only our additions to prove the complete previous document survives.
    restored=deepcopy(z)
    for path, key in [('objects','world'),('materials',None),('formationRelations',None),('lawRefs',None),('lexemes',None)]:
        source=old[key] if key else old
        dest=restored[key] if key else restored
        if path in source: dest[path]=dest[path][:len(source[path])]
        else: dest.pop(path,None)
    assert restored==old, 'An existing authored value changed'


def install(root):
    package=build()
    paths=[root/'zones'/ZONE/'zone.json',root/'worlds/cathedral_of_the_living_logos.json']
    before={p:p.read_bytes() for p in paths}
    docs={p:json.loads(data) for p,data in before.items()}
    append_zone(docs[paths[0]],package)
    world=docs[paths[1]]
    append_zone(next(z for z in world['zones'] if z['identifier']==ZONE),package)
    authored=world['authoredLaws']
    for l in package['laws']:
        assert not any(x['id']==l['identifier'] for x in authored['laws'])
        authored['laws'].append(l['law'])
        authored['formationMembers'].append(l['identifier'])
        if l['triggers']: authored['triggers'][l['identifier']]=l['triggers']
        p=root/'laws'/l['identifier']/'law.json'
        if p.exists(): raise ValueError('Law already exists: '+str(p))
        docs[p]=l
    # Optimistic check immediately before committing. A backup is kept outside
    # SaveRoot, so a recoverable interruption cannot be mistaken for another Zone.
    backup=Path(tempfile.mkdtemp(prefix='earthcall-openhand-before-'))
    for p,data in before.items():
        if p.read_bytes()!=data: raise RuntimeError('Concurrent edit; rerun safely: '+str(p))
        target=backup/p.relative_to(root);target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
    staged={}
    for p,d in docs.items():
        p.parent.mkdir(parents=True,exist_ok=True)
        with tempfile.NamedTemporaryFile(mode='w',dir=p.parent,prefix='.openhand-',delete=False) as f:
            json.dump(d,f,indent=2);f.write('\n');staged[p]=Path(f.name)
    for p,data in before.items():
        if p.read_bytes()!=data: raise RuntimeError('Concurrent edit while staging: '+str(p))
    # Dependencies first, then session fallback, then authoritative Zone identity.
    for p in [p for p in staged if p not in paths]+[paths[1],paths[0]]:
        staged[p].replace(p)
    print(json.dumps({'addedObjects':len(package['objects']),'addedMaterials':len(package['materials']),
                      'addedLaws':len(package['laws']),'backup':str(backup),
                      'beforeSha256':{str(p):hashlib.sha256(v).hexdigest() for p,v in before.items()}},indent=2))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--save-root',type=Path,default=ROOT/'saves')
    parser.add_argument('--package',type=Path,help='Export addition only; do not modify a Zone')
    args=parser.parse_args()
    if args.package:
        args.package.write_text(json.dumps(build(),indent=2)+'\n')
    else:
        install(args.save_root)
