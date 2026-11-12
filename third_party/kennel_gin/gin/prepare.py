"""Generate Gin's coefficient data and private R246 font/prefix inputs locally.
No firmware/font-derived bytes belong in corresponding source.
Copyright (C) 2026 Hundekraft. GPL-3.0-or-later with OP-1 firmware permission.
"""
from pathlib import Path
import importlib.util,json,subprocess,sys,hashlib
# Python 3.9's system SQLite lacks serialize/deserialize. Keep the original
# builders usable through SQLite's backup API rather than adding a runtime dependency.
import sqlite3,tempfile
class Connection(sqlite3.Connection):
 def serialize(self):
  with tempfile.TemporaryDirectory() as d:
   p=Path(d)/'db';other=sqlite3.connect(str(p));self.backup(other);other.close();return p.read_bytes()
 def deserialize(self,data):
  with tempfile.TemporaryDirectory() as d:
   p=Path(d)/'db';p.write_bytes(data);other=sqlite3.connect(str(p));other.backup(self);other.close()
def connect(path):return sqlite3.connect(path,factory=Connection)
def catalog_module(v):
 from types import SimpleNamespace
 mod=load('gin_drum_catalog',v/'tools/tonic/sdk-drum/drum_catalog.py');mod.sqlite3=SimpleNamespace(connect=connect);return mod
HOOKS=[(0x01967984,'dsfactory',14),(0x019b9810,'dstype',14),(0x01a037a8,'dsnormalize',4),(0x01a3bfee,'dsscreen',6),(0x019fffba,'dsjsonsave',4),(0x01a00630,'dsjsonload',4),(0x01a0502a,'dspresetquery',4),(0x0193f5c2,'dspresetcopy',4),(0x019ad554,'dssnapread',4),(0x019ad776,'dssnapsave',4)]
def load(name,path):
 spec=importlib.util.spec_from_file_location(name,path);mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod);return mod
def bank_description(v):
 bank=json.loads((v/'src/tonic/factory_bank/bank.json').read_text())
 for i,kit in enumerate(bank['kits']):kit['order']=i
 return bank
def prepare(sdk,stock,hooks):
 sdk,stock=Path(sdk),Path(stock);v=sdk/'plugins/gin/vendor';out=sdk/'build/gin';out.mkdir(parents=True,exist_ok=True)
 if not (stock/'OP1_vdk.ldr').exists():
  import io,tarfile,lzma,zlib,struct
  archive=next((p/'firmware/op1_246.op1' for p in sdk.parents if (p/'firmware/op1_246.op1').exists()),None)
  if archive is None:raise ValueError('Qualified R246 is required for native drum-synth build inputs')
  raw=archive.read_bytes()
  if hashlib.sha256(raw).hexdigest()!='c5315218f825f143b415ca554516541898abee843d3a236df0b54c04e1fb13a9':raise ValueError('Original R246 required')
  stock=out/'stock-private'
  with tarfile.open(fileobj=io.BytesIO(lzma.decompress(raw[4:],format=lzma.FORMAT_ALONE))) as tar:
   for name in ['OP1_vdk.ldr','content/op1_factory.db','content/display/opfont.svg']:
    target=stock/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(tar.extractfile(name).read())
 sys.path.insert(0,str(v/'tools/tonic/release'))
 assets=load('gin_stock_assets',v/'tools/tonic/release/prepare_stock_assets.py')
 app=(stock/'OP1_vdk.ldr').read_bytes()
 if hashlib.sha256(app).hexdigest()!=assets.APP_SHA:raise ValueError('Gin requires qualified original R246')
 for addr,name,size in HOOKS:
  original=assets.image_read(app,addr,size)
  if addr in hooks and hooks[addr]!=('hook_'+name,original):raise ValueError('Duplicate drum-synth hook owner: '+hex(addr))
  hooks[addr]=('hook_'+name,original)
 for name,addr,size in [('type',0x019b9810,14),('normalize',0x01a037a8,4)]:
  (sdk/'build'/('ds_'+name+'.private.inc')).write_text('.byte '+','.join(str(b)for b in assets.image_read(app,addr,size))+'\n')
 if not (out/'gin_font.private.inc').exists():
  font=(stock/'content/display/opfont.svg').read_bytes();(out/'font.private.svg').write_bytes(font)
  # glyph boxes in Python (font_bounds.py, as Chromium's getBBox), not the vendor's font_bounds.cjs (Node + Playwright)
  bounds=load('gin_font_bounds',Path(__file__).with_name('font_bounds.py')).bounds(font)
  (out/'bounds.private.json').write_text(json.dumps(bounds))
  geometry=load('gin_font_geometry',v/'tools/tonic/release/font_geometry.py')
  header=geometry.generate(assets.glyph_nodes(font,bounds))
  if hashlib.sha256(header).hexdigest()!=assets.HEADER_SHA:raise ValueError('Gin font geometry does not match the accepted header')
  (out/'gin_font.private.inc').write_bytes(header)
 sys.path.pop(0)
 boot=out/'bootstrap';factory=out/'factory'
 if not boot.exists():load('gin_bootstrap',v/'tools/tonic/release/build_bootstrap_data.py').build(v/'bootstrap',boot)
 if not factory.exists():
  sys.path.insert(0,str(v/'tools/tonic/sdk-drum'))
  gen=load('gin_factory_generation',v/'tools/tonic/sdk-drum/build_factory_data.py');gen.ROOT=v.resolve();gen.build(factory)
  sys.path.pop(0)
 names={'gin_fw_builtin_kit':boot/'builtin.kit.bin','gin_fw_builtin_prepared':boot/'builtin.prepared.bin','gin_fw_builtin_patch':boot/'builtin.patch.bin','gin_factory_prepared_dictionary':factory/'factory.dictionary.bin','gin_factory_prepared_index':factory/'factory.index12.bin'}
 text='.section .rodata\n'
 for name,path in names.items():text+=f'.balign 4\n.global _{name}\n_{name}:\n.incbin "{path.resolve()}"\n'
 (out/'gin_data.S').write_text(text)
 # Regenerate kit-browser rows when installing this file into a previously
 # Gin-free build. Derived common metadata is generated from qualified R246.
 import sqlite3
 catalog_builder=catalog_module(v)
 db=connect(':memory:');db.deserialize((stock/'content/op1_factory.db').read_bytes())
 data,_=catalog_builder.patch_gin_presets(db.serialize(),bank_description(v))
 db.deserialize(data);statements=[]
 for patch,folder in db.execute("SELECT patch,folder FROM drum_presets WHERE folder='gin' ORDER BY id"):
  name=json.loads(patch)['name'];q=lambda value: "'"+value.replace("'","''")+"'"
  statements.append("INSERT INTO drum_presets(id,patch,folder) SELECT (SELECT COALESCE(MAX(id),-1)+1 FROM drum_presets),"+q(patch)+","+q(folder)+" WHERE NOT EXISTS (SELECT 1 FROM drum_presets WHERE folder="+q(folder)+" AND patch LIKE "+q('%"name":'+json.dumps(name)+'%')+");")
 db.close()
 (out/'gin_catalog.inc').write_text('static const char *const gin_catalog_sql[]={'+','.join(json.dumps(x) for x in statements)+',0};\n')

def catalog(connection,sdk):
 v=Path(sdk)/'plugins/gin/vendor';mod=catalog_module(v)
 mod.register_drums(connection,[{'name':'gin','drum_defaults':[0]*8}]);connection.commit()
 if not connection.execute("SELECT 1 FROM drum_presets WHERE folder='gin'").fetchone():
  import sqlite3
  bank=bank_description(v)
  data,_=mod.patch_gin_presets(Path(connection.execute('PRAGMA database_list').fetchone()[2]).read_bytes(),bank)
  other=connect(':memory:');other.deserialize(data)
  for row in other.execute("SELECT id,patch,folder FROM drum_presets WHERE folder='gin'"):connection.execute('INSERT INTO drum_presets VALUES(?,?,?)',row)
  other.close()
