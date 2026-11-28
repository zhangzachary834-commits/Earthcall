"""Generate a pasteable authored editor; never modify a save or install an app.
Zach requested a whole 2D art editor through Law Line. Codex / GPT-6.1 Sol,
session 01a10992-828e-7e80-890c-c64b09141e18; request 2026-10-07, native final
2026-10-08T07:19:33Z. The Python is a First Mover
text-authoring aid only: every editor decision lives in the emitted Laws.
"""
from pathlib import Path

N = 16
ROOT = 'my.atelier'
SCREEN = '@screen-channel'
INPUT = '@interaction-channel'
PALETTE = [(0.08,0.10,0.16),(1,1,1),(1,.25,.35),(1,.55,.15),(1,.84,0),(.35,.85,.45),
           (0,.8,.85),(.2,.5,1),(.5,.35,.95),(.9,.35,.8),(.55,.32,.18),(.7,.75,.85)]

def num(x): return f'{x:.8g}'
def vector(c): return '('+', '.join(num(x) for x in c)+')'
def selector(x0,y0,x1,y1):
    return ('Intersection <a: $(Abs <value: $(u - '+num((x0+x1)/2)+')> - '+num((x1-x0)/2)+'), '
            'b: $(Abs <value: $(v - '+num((y0+y1)/2)+')> - '+num((y1-y0)/2)+')>')
def piece(c, rect=None):
    return 'Piece <value: $('+vector(c)+')'+(', where: $('+selector(*rect)+')' if rect else '')+'>'
def seq(actions): return 'Sequence <children: ['+', '.join(actions)+']>'
def law(name,cond,actions,mode='becomes true'):
    return 'called "Atelier '+name+'" '+mode+' if '+cond+' then '+seq(actions)
def rectangle(x0,y0,x1,y1):
    return ' and '.join(f'{INPUT}.pointer{axis} {op} {num(v)}' for axis,op,v in
                        [('U','>=',x0),('U','<',x1),('V','>=',y0),('V','<',y1)])
def hit(rect):
    return 'is a Person and '+ROOT+'.enabled is true and '+INPUT+'.pointerLocked is false and '+INPUT+'.leftDown is true and '+rectangle(*rect)
def field_path(i):
    return f'{ROOT}.canvas.astDefinition.pieces.{i}.mathNode'
def ink(c):return 'Piece <value: $('+vector(c)+')>'
def checkpoint():
    return [f'set {ROOT}.undo to {ROOT}.canvas',f'set {ROOT}.canUndo to true',f'set {ROOT}.canRedo to false']
def program():
    cells=[]; sentences=[]
    for y in range(N):
        for x in range(N):
            x0=.18+.62*x/N; y0=.12+.76*y/N
            rect=(x0,y0,x0+.62/N,y0+.76/N)
            # Slight authored inset makes the pixel grid visible. Picking
            # uses the whole cell, so its border still addresses that cell.
            cells.append(piece((1,1,1),(rect[0]+.0007,rect[1]+.0007,rect[2]-.0007,rect[3]-.0007)))
            actions=checkpoint()+[f'set {field_path(y*N+x)} to {ROOT}.brush.mathNode']
            sentences.append(law(f'Pixel {x} {y}',hit(rect),actions))
    for i,c in enumerate(PALETTE):
        rect=(.055,.12+i*.059,.125,.12+i*.059+.043)
        cells.append(piece(c,rect))
        sentences.append(law(f'Ink {i}',hit(rect),[f'add property {ROOT}.brush to '+ink(c)]))
    # Visible action tiles; symbols are themselves authored geometric pieces.
    # Ordered before the tile backgrounds because Piecewise is first-match.
    icons=[]
    def bar(rect,c=(1,1,1)): icons.append(piece(c,rect))
    # Undo left arrow, redo right arrow, clear cross, PNG down arrow, close X.
    for y,left in [(.20,True),(.33,False)]:
        bar((.865,y-.005,.92,y+.005))
        for k in range(4):
            x=.865+k*.006 if left else .914-k*.006
            bar((x,y-.005-k*.006,x+.006,y+.005+k*.006))
    for y in [.46,.85]:
        for k in range(7):
            x=.863+k*.008
            bar((x,y-.025+k*.008,x+.008,y-.017+k*.008))
            bar((x,y+.023-k*.008,x+.008,y+.031-k*.008))
    bar((.887,.56,.897,.61))
    for k in range(4): bar((.869+k*.006,.596+k*.006,.915-k*.006,.602+k*.006))
    tiles=[(.20,(.30,.42,.70)),(.33,(.30,.42,.70)),(.46,(.85,.32,.38)),(.59,(.15,.62,.55)),(.72,(.85,.85,.85)),(.85,(.35,.37,.43))]
    static=icons+[piece(c,(.84,y-.047,.945,y+.047)) for y,c in tiles]
    # Eraser symbol, dark horizontal centre on the white tile.
    static.insert(0,piece((.3,.32,.38),(.865,.715,.922,.725)))
    cells=static+cells+[piece((.055,.068,.11))]
    shift=len(static)
    # Use canonical indices after inserting static UI. No runtime method picks
    # a pixel, allocates a brush, decides a tool, or stores private app state.
    for i,line in enumerate(sentences[:N*N]):
        line=line.replace(field_path(i),field_path(i+shift))
        sentences[i]=line
    source='VectorField <pieces: ['+', '.join(cells)+']>'
    setup=law('Install','is a Person and not '+ROOT+'.installed is true',[
        'remove property '+SCREEN+'.output.colorPath','remove property '+SCREEN+'.output.opacityPath',
        'remove property '+SCREEN+'.output.opacity','remove property '+SCREEN+'.output.timePath',
        'remove property '+SCREEN+'.output.time','remove property '+SCREEN+'.sample.request',
        *[f'add property {ROOT}.{key} to {value}' for key,value in
          [('installed','true'),('enabled','true'),
           ('canUndo','false'),('canRedo','false'),('undo','0'),('redo','0'),('blank','0')]],
        f'add property {ROOT}.canvas to '+source,
        f'add property {ROOT}.brush to '+ink((1,.84,0)),
        f'set {ROOT}.blank to {ROOT}.canvas',
        f'add property {SCREEN}.output.color to 0', f'set {SCREEN}.output.color to {ROOT}.canvas'])
    display=law('Display','is a Person and '+ROOT+'.enabled is true',[
        f'set {SCREEN}.output.color to {ROOT}.canvas'],mode='always')
    actions=[
        law('Undo',hit((.84,.153,.945,.247))+' and '+ROOT+'.canUndo is true',[
            f'set {ROOT}.redo to {ROOT}.canvas',f'set {ROOT}.canvas to {ROOT}.undo',f'set {ROOT}.canUndo to false',f'set {ROOT}.canRedo to true']),
        law('Redo',hit((.84,.283,.945,.377))+' and '+ROOT+'.canRedo is true',[
            f'set {ROOT}.undo to {ROOT}.canvas',f'set {ROOT}.canvas to {ROOT}.redo',f'set {ROOT}.canUndo to true',f'set {ROOT}.canRedo to false']),
        law('Clear',hit((.84,.413,.945,.507)),checkpoint()+[f'set {ROOT}.canvas to {ROOT}.blank']),
        law('PNG',hit((.84,.543,.945,.637)),['set @screen-recorder.recordCursor to false','set @screen-recorder.snapshot to true']),
        law('Eraser',hit((.84,.673,.945,.767)),[f'add property {ROOT}.brush to '+ink((1,1,1))]),
        law('Close',hit((.84,.803,.945,.897)),[f'set {ROOT}.enabled to false','remove property '+SCREEN+'.output.color','remove property '+SCREEN+'.output.colorPath']),
    ]
    return '; '.join([setup,display]+sentences+actions)+'\n',shift

if __name__ == '__main__':
    text,shift=program()
    target=Path(__file__).resolve().parents[1]/'examples/law_line_pixel_art_editor.txt'
    target.write_text(text)
    print(f'Authored {N}x{N} editor: {len(text.encode())} bytes, {text.count("; ")+1} cooperating Laws, first cell piece {shift}. No saves modified.')
