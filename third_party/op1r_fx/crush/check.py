#!/usr/bin/env python3
"""Focused native DSP and unclipped screen checks; no emulator or firmware extraction."""
import itertools,json,subprocess,sys,tempfile
from pathlib import Path
HERE=Path(__file__).resolve().parent
SDK=HERE.parents[1]
sys.path.insert(0,str(SDK/'harness'))
import layout

def main():
    with tempfile.TemporaryDirectory(prefix='crush-check-') as tmp:
        tmp=Path(tmp)
        flags=['cc','-O1','-g','-Wall','-Wextra','-fsanitize=address,undefined','-fno-sanitize-recover=undefined',f'-I{SDK}']
        for name in ('crush_test','crush_screen_test'):
            subprocess.run(flags+[str(SDK/'harness'/f'{name}.c'),str(HERE/'crush_dsp.c'),'-lm','-o',str(tmp/name)],check=True)
        subprocess.run([str(tmp/'crush_test')],check=True)
        states=[(10922,21239,8192,32767),*itertools.product((0,32767),repeat=4)]
        states += [(10922,21239,8192,32767,1),(0,0,0,0,1),(32767,32767,32767,32767,1)]
        contract={'allowed':[[0,0,320,160]],'protected':[]}
        for q in states:
            frame=json.loads(subprocess.check_output([str(tmp/'crush_screen_test'),*map(str,q)]))
            frame['svg']=str(HERE/'crush.svg')
            result=layout.check_bounds(frame,contract)
            if result['status']!='pass':raise AssertionError((q,result))
        print(f'Crush {len(states)} unclipped full-screen native-font model fixtures PASS (sizes 14/40 uncalibrated)')
if __name__=='__main__':main()
