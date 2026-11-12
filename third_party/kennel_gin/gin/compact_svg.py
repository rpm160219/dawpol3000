"""Lossless adjacent equal-path fill/stroke merge; preserve scene IDs and geometry."""
from pathlib import Path
import re
root=Path(__file__).parent
source=(root/'vendor/src/tonic/ui_native/gin.svg').read_text()
compact,count=re.subn(r'<path d="([^\"]+)" fill="#000000"/><path d="\1" fill="none" stroke=',r'<path d="\1" fill="#000000" stroke=',source)
if count!=2:raise ValueError('Gin source SVG geometry changed; inspect before regenerating')
(root/'gin.svg').write_text(compact)
