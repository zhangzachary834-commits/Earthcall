"""Author Shape Garden as Law Line text, never as application C++.
Zach commissioned a small 2D sidebar / 3D generator made of Singulars and Laws.
Codex / GPT-6.1 Sol / 01a10992-828e-7e80-890c-c64b09141e18 / 2026-10-20.
No save is modified by this text generator. See the installation/test runner.
"""
from pathlib import Path
import json

ZONE = 'ShapeGarden'
ROOT = '@ShapeGarden.forge'
INPUT = '@interaction-channel'
COLORS = [(0.15,.78,.87),(.98,.63,.22),(.58,.42,.93),(.92,.30,.48)]
SHAPES = {
    'SPHERE': 'sqrt(x*x+y*y+z*z)-0.72',
    'BOX': '',
    'TORUS': 'sqrt((sqrt(x*x+z*z)-0.7)*(sqrt(x*x+z*z)-0.7)+y*y)-0.23',
}
def mx(a,b): return f'(({a})+({b})+abs(({a})-({b})))/2'
SHAPES['BOX'] = mx('abs(x)-0.66', mx('abs(y)-0.66','abs(z)-0.66'))
FLOOR = mx('abs(x)-12', mx('abs(y)-0.12','abs(z)-12'))
def n(v): return f'{v:.7g}'
def vec(v): return '('+', '.join(map(n,v))+')'
def quote(v): return json.dumps(v)
def seq(actions): return 'Sequence <children: ['+', '.join(actions)+']>'
def rectangle(r, u='u', v='v'):
    x,y,w,h=r
    return ('Intersection <a: $(Abs <value: $('+u+' - '+n(x+w/2)+')> - '+n(w/2)+'), '
            'b: $(Abs <value: $('+v+' - '+n(y+h/2)+')> - '+n(h/2)+')>')
def piece(c,r=None): return 'Piece <value: $('+vec(c)+')'+(', where: $('+rectangle(r)+')' if r else '')+'>'
def scalar_piece(value,r=None): return 'Piece <value: $('+n(value)+')'+(', where: $('+rectangle(r)+')' if r else '')+'>'
def field(pieces, vector=True):
    return ('VectorField' if vector else 'ScalarField')+' <pieces: ['+', '.join(pieces)+']>'
# Tiny authored stroke glyphs: their rectangles are ordinary mathematical form,
# not Text2D/ShapeKind carriers or an engine-side label renderer.
FONT={
 'A':['010','101','111','101','101'], 'B':['110','101','110','101','110'],
 'C':['011','100','100','100','011'], 'E':['111','100','110','100','111'],
 'F':['111','100','110','100','100'], 'G':['011','100','101','101','011'],
 'H':['101','101','111','101','101'], 'I':['111','010','010','010','111'],
 'L':['100','100','100','100','111'], 'M':['101','111','111','101','101'],
 'O':['010','101','101','101','010'], 'P':['110','101','110','100','100'],
 'R':['110','101','110','101','101'], 'S':['011','100','010','001','110'],
 'T':['111','010','010','010','010'], 'U':['101','101','101','101','111'],
 'X':['101','101','010','101','101'], 'Z':['111','001','010','100','111'],
 '+':['000','010','111','010','000'], '-':['000','000','111','000','000'],
}
def text(word,x,y,p=.0033,color=(.83,.87,.94)):
    out=[]
    for k,ch in enumerate(word):
        for j,row in enumerate(FONT.get(ch,['000']*5)):
            for i,bit in enumerate(row):
                if bit=='1': out.append(piece(color,(x+(k*4+i)*p,y+j*p,p*.8,p*.8)))
    return out

def program():
    laws=[]; pieces=[]; highlights={}; selected=[]
    def law(name,cond,actions,mode='becomes true'):
        laws.append('called '+quote('Shape Garden | '+name)+' '+mode+' if is a Person and '+cond+' then '+seq(actions))
    def setp(k,v): return 'set '+ROOT+'.'+k+' to '+v
    def add(k,v): return 'add property '+ROOT+'.'+k+' to '+v
    def tint(index,c):
        return ['set '+ROOT+f'.panel.astDefinition.pieces.{index}.mathNode.children.{j}.scalarForm.terms.0.c to '+n(v) for j,v in enumerate(c)]
    bg=(.032,.045,.078); tile=(.075,.095,.14); gold=(.98,.70,.32); muted=(.22,.28,.37)
    pieces+=text('FORM',.048,.104,.006,gold)
    pieces+=text('X',.181,.11,.0035,muted)
    for i,shape in enumerate(SHAPES):
        y=.205+i*.108
        pieces+=text(shape,.066,y+.020,.0037)
        selected.append(len(pieces));pieces.append(piece(gold if i==0 else muted,(.045,y,.005,.065)))
        pieces.append(piece(tile,(.054,y,.135,.065)))
    pieces+=text('SIZE',.05,.556,.004)
    for symbol,x in [('-',.05),('+',.145)]:
        pieces+=text(symbol,x+.016,.596,.006)
        pieces.append(piece(tile,(x,.58,.042,.059)))
    # Three little bars make the chosen size visible without a text widget.
    size_marks=[]
    for i in range(3):
        size_marks.append(len(pieces));pieces.append(piece(gold if i<2 else muted,(.101,.591+i*.014,.028,.007)))
    pieces+=text('COLOR',.05,.681,.0038)
    color_marks=[]
    for i,c in enumerate(COLORS):
        color_marks.append(len(pieces));pieces.append(piece(gold if i==0 else muted,(.047+i*.038,.77,.028,.004)))
        pieces.append(piece(c,(.047+i*.038,.722,.028,.041)))
    pieces+=text('CREATE',.058,.814,.0047,(.025,.035,.055))
    pieces.append(piece(gold,(.043,.79,.15,.07)))
    pieces.append(piece(bg,(.027,.082,.186,.816)))
    pieces.append(piece((.14,.19,.28),(.025,.08,.19,.82)))
    # First-applicable Piece wins: foreground, controls, frame, background.
    panel=field(pieces)
    opacity=field([scalar_piece(.96,(.025,.08,.19,.82)), scalar_piece(0)],False)
    init=[add('installed','true'),add('enabled','true'),add('ready','true'),add('pressU','-1'),add('pressV','-1'),
          add('shape',quote('SPHERE')),add('expression',quote(SHAPES['SPHERE'])),add('color',' '.join(map(n,COLORS[0]))),add('rotation','0 0 0'),
          add('size','1'),add('sizeIndex','2'),add('nextX','-1.2'),add('nextZ','0'),add('count','0'),
          add('panel',panel),add('opacity',opacity),
          'add property @screen-channel.output.colorPath to '+quote(ROOT+'.panel'),
          'add property @screen-channel.output.opacityPath to '+quote(ROOT+'.opacity')]
    # A real floor, authored by the same universal Create operation as the
    # generated solids. Only scalar math chooses the geometry, never ShapeKind.
    init.append('Create <Object, properties: {field.expr: '+quote(FLOOR)+', field.extent: (12.1, 0.2, 12.1), position: (0, -0.12, 0), color: (0.18, 0.21, 0.27), authored: {shapeGardenFloor: true}}>')
    law('Install','not '+ROOT+'.installed is true',init)
    enabled=ROOT+'.enabled is true'
    law('Bind sidebar',enabled,['add property @screen-channel.output.colorPath to '+quote(ROOT+'.panel'), 'add property @screen-channel.output.opacityPath to '+quote(ROOT+'.opacity')], 'always')
    # Capture the press origin once; dragging into a tile is not another click.
    law('Press origin',enabled+' and '+INPUT+'.pointerLocked is false and '+INPUT+'.leftDown is true',
        [setp('pressU',INPUT+'.pointerU'),setp('pressV',INPUT+'.pointerV')])
    law('Release',ROOT+'.installed is true and '+INPUT+'.leftDown is false',[setp('ready','true'),setp('pressU','-1'),setp('pressV','-1')], 'always')
    def hit(r):
        x,y,w,h=r
        return enabled+' and '+ROOT+'.ready is true and '+INPUT+'.pointerLocked is false and '+INPUT+'.leftDown is true and '+ ' and '.join(ROOT+'.press'+axis+' '+op+' '+n(value) for axis,op,value in [('U','>=',x),('U','<',x+w),('V','>=',y),('V','<',y+h)])
    for i,(shape,expr) in enumerate(SHAPES.items()):
        actions=[setp('ready','false'),setp('shape',quote(shape)),setp('rotation',{'SPHERE':'0 0 0','BOX':'15 25 0','TORUS':'60 0 0'}[shape])]
        for j,index in enumerate(selected): actions+=tint(index,gold if i==j else muted)
        law('Choose '+shape,hit((.043,.205+i*.108,.15,.065)),actions)
    for i,c in enumerate(COLORS):
        actions=[setp('ready','false'),setp('color',' '.join(map(n,c)))]
        for j,index in enumerate(color_marks): actions+=tint(index,gold if i==j else muted)
        law('Choose color '+str(i),hit((.047+i*.038,.722,.028,.041)),actions)
    for i,factor in enumerate([.6,1,1.4],1):
        law('Size '+str(i),ROOT+f'.sizeIndex == {i}',[setp('size',n(factor))]+sum([tint(index,gold if j<i else muted) for j,index in enumerate(size_marks)],[]))
    law('Smaller',hit((.05,.58,.042,.059))+' and '+ROOT+'.sizeIndex > 1',[setp('ready','false'),'add '+ROOT+'.sizeIndex by -1'])
    law('Larger',hit((.145,.58,.042,.059))+' and '+ROOT+'.sizeIndex < 3',[setp('ready','false'),'add '+ROOT+'.sizeIndex by 1'])
    for shape,expr in SHAPES.items():
        for i,factor in enumerate([.6,1,1.4],1):
            scaled=expr
            for axis in ['x','y','z']: scaled=scaled.replace(axis,'('+axis+'/'+n(factor)+')')
            scaled=n(factor)+'*('+scaled+')'
            law('Form '+shape+' '+str(i),ROOT+'.shape == '+quote(shape)+' and '+ROOT+f'.sizeIndex == {i}',[setp('expression',quote(scaled))])
    # Spawn in front of the Person, with horizontal offsets along their view's
    # right vector. This reads perception, never actuates the Person's body.
    distance='(5 + '+ROOT+'.nextZ)'
    px='my.position.x + my.cameraForward.x * '+distance+' - my.cameraForward.z * '+ROOT+'.nextX'
    pz='my.position.z + my.cameraForward.z * '+distance+' + my.cameraForward.x * '+ROOT+'.nextX'
    create='Create <Object, properties: {field.expr: '+ROOT+'.expression, field.extent: (1.6, 1.6, 1.6), position: ('+px+', my.position.y + 1.2, '+pz+'), rotation: '+ROOT+'.rotation, color: '+ROOT+'.color, authored: {shapeGardenGenerated: true}}>'
    law('Create solid',hit((.043,.79,.15,.07)),[setp('ready','false'),create,'add '+ROOT+'.count by 1','add '+ROOT+'.nextX by 1.8'])
    law('Next row',ROOT+'.nextX > 2.5',[setp('nextX','-1.2'),'add '+ROOT+'.nextZ by 2.6'])
    law('Close sidebar',hit((.174,.10,.03,.04)),[setp('ready','false'),setp('enabled','false'),
        'remove property @screen-channel.output.colorPath','remove property @screen-channel.output.opacityPath',
        'remove property @screen-channel.output.color','remove property @screen-channel.output.opacity'])
    return laws

if __name__=='__main__':
    root=Path(__file__).resolve().parents[1]
    laws=program()
    (root/'examples/law_line_shape_garden.txt').write_text('; '.join(laws)+'\n')
    (root/'examples/law_line_shape_garden_sentences.json').write_text(json.dumps(laws,indent=2)+'\n')
    print(f'Authored {len(laws)} Law sentences; no save modified.')
