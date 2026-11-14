#!/usr/bin/env python3
"""Convert the supplied Polivoks concept to native compound vector contours.

Offline authoring only: pip install numpy pillow opencv-python-headless.
Usage: python trace_polivoks.py reference.jpeg core/ui/polivoks_art.inc
The shipped application needs neither the JPEG nor Python/image libraries.
"""
import argparse
import hashlib
from pathlib import Path
import cv2
import numpy as np
from PIL import Image

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('reference', type=Path)
parser.add_argument('output', type=Path)
args = parser.parse_args()
image = np.asarray(Image.open(args.reference).convert('RGB')).copy()
assert image.shape == (1792, 2400, 3), 'Expected original 2400 x 1792 concept'
# The scope obscures the right cheek/chin/coat. Restore the hidden contours
# from the corresponding visible left half before rendering the live scope.
yy, xx = np.indices(image.shape[:2])
beam = ((xx >= 1200) & (xx < 1910) &
        (yy > 960 - (xx - 1190) * .27) &
        (yy < 960 + (xx - 1190) * .72))
image[beam] = image[yy[beam], 2399 - xx[beam]]
# Remove every colored remnant of the source scope from the central artwork.
# The live beam is a separate layer with variable width; retaining any of the
# old green/blue contours makes a second branch escape at low cutoff / BP.
col=image.astype(np.float32)
cool=((col[:,:,1]>col[:,:,0]+10)&(col[:,:,1]>col[:,:,2]*1.07)) | ((col[:,:,2]>col[:,:,0]+25)&(col[:,:,1]>col[:,:,0]+8)&(col[:,:,2]>col[:,:,1]*1.1))
central=(xx>=550)&(xx<1950)&(yy>=770)&(yy<1292)
image[central & cool]=0
# Only the illustration, without the reference's header, captions or controls.
keep = (yy >= 192) & (yy <= 1292) & (xx >= 68) & (xx <= 2330)
keep &= ~((xx < 205) & (yy > 680))
keep &= ~((xx > 2200) & (yy > 680))
keep &= ~((xx < 550) & (yy > 1110) & (yy < 1180))
keep &= ~((xx > 1950) & (yy > 1110) & (yy < 1180))
keep &= ~((yy > 1195) & ((xx < 950) | (xx > 1450)))
# Exclude the original horizontal separator: the native scene draws its own.
keep &= ~((yy >= 1185) & (yy <= 1195) & ((xx < 950) | (xx > 1450)))
image[~keep] = 0
flat = image.reshape(-1,3).astype(np.float32)
bright = np.max(flat,axis=1)>26
# Palette locked to the accepted reference trace: local scope cleanup must
# not change the colors of the hat, face or fur.
centers=np.array([
    [7.825464248657227,16.111454010009766,36.30533981323242],
    [37.21607971191406,8.478482246398926,9.730573654174805],
    [4.091736316680908,40.777862548828125,22.12332534790039],
    [45.68306350708008,37.69676208496094,32.02677536010742],
    [12.207180976867676,25.522064208984375,58.474571228027344],
    [61.31291961669922,11.702746391296387,15.438453674316406],
    [6.221927642822266,55.15694046020508,65.76171875],
    [71.20668029785156,65.13069915771484,59.97385787963867],
    [3.8688554763793945,76.1153793334961,39.502689361572266],
    [15.879566192626953,34.558738708496094,85.84353637695312],
    [87.14926147460938,16.76192855834961,22.36701774597168],
    [104.18122863769531,84.66912078857422,79.86721801757812],
    [79.94489288330078,70.9397964477539,104.81121826171875],
    [105.74072265625,103.22367858886719,99.44622802734375],
    [20.090978622436523,45.359771728515625,110.05442810058594],
    [116.33528900146484,20.807836532592773,27.75533103942871],
    [137.2283172607422,135.78720092773438,132.2813262939453],
    [143.43807983398438,120.0599594116211,93.98523712158203],
    [22.15191650390625,144.36328125,80.1572036743164],
    [148.3650360107422,38.356536865234375,40.573856353759766],
    [35.36101150512695,77.19674682617188,155.84295654296875],
    [160.73867797851562,135.03414916992188,107.89030456542969],
    [181.056640625,168.34169006347656,161.14019775390625],
    [190.19908142089844,58.592716217041016,60.38225173950195],
    [65.60554504394531,116.22296142578125,202.3456573486328],
    [36.693328857421875,207.56629943847656,116.04995727539062],
    [219.93492126464844,80.09237670898438,81.43597412109375],
    [231.92062377929688,218.66152954101562,205.78152465820312],
    [244.929443359375,69.25279235839844,71.52645111083984],
    [81.81857299804688,141.2770233154297,245.89712524414062],
    [246.36068725585938,156.3874053955078,151.85496520996094],
    [247.58071899414062,241.2020263671875,235.89686584472656],
],dtype=np.float32)
# Memory-bounded nearest-palette assignment.
labels = np.full(len(flat), -1, np.int16)
idx = np.flatnonzero(bright)
for start in range(0,len(idx),32000):
    at=idx[start:start+32000]
    labels[at] = np.argmin(np.sum((flat[at,None,:]-centers[None,:,:])**2,axis=2),axis=1)
labels=labels.reshape(image.shape[:2])
points=[]; shapes=[]
for k,center in enumerate(centers):
    mask=np.uint8(labels==k)*255
    count,components,stats,_=cv2.connectedComponentsWithStats(mask,8)
    allowed=stats[:,cv2.CC_STAT_AREA]>=6
    allowed[0]=False
    mask=np.uint8(allowed[components])*255
    # Subpixel palette boundaries share coverage when rasterized at 1/4 scale.
    # A one-source-pixel overlap prevents dark seams between adjacent inks.
    mask=cv2.dilate(mask,np.ones((3,3),np.uint8))
    # Palette quantization can otherwise turn faint scope tint into saturated
    # green/blue again. These inks belong only to the coils in the static art.
    r,g,b=map(float,center)
    cool_ink=(g>r+10 and g>b*1.07) or (b>r+25 and g>r+8 and b>g*1.1)
    if cool_ink: mask[central]=0
    contours,hierarchy=cv2.findContours(mask,cv2.RETR_CCOMP,cv2.CHAIN_APPROX_SIMPLE)
    if hierarchy is None: continue
    hierarchy=hierarchy[0]
    for i,c in enumerate(contours):
        if hierarchy[i,3]>=0 or abs(cv2.contourArea(c))<3: continue
        outer=cv2.approxPolyDP(c,.55,True)[:,0,:].tolist()
        if len(outer)<3: continue
        compound=outer+[outer[0]]
        hole=hierarchy[i,2]
        while hole>=0:
            h=cv2.approxPolyDP(contours[hole],.55,True)[:,0,:].tolist()
            if len(h)>=3 and abs(cv2.contourArea(contours[hole]))>=2:
                compound+=h+[h[0],outer[0]]
            hole=hierarchy[hole,0]
        start=len(points)
        points.extend(compound)
        shapes.append((start,len(compound),*np.uint8(np.rint(center)).tolist()))
lines=['// Native vector contours traced from the user-supplied Polivoks concept.',
       '// Generated by tools/art/trace_polivoks.py; source JPEG is not a runtime asset.',
       '// Source SHA-256: '+hashlib.sha256(args.reference.read_bytes()).hexdigest(),
       'struct BearContour { unsigned start; unsigned short count; Color color; };',
       'static constexpr int kBearMaxPoints = %d;'%max(s[1] for s in shapes),
       'static const int16_t kBearPoints[][2] = {']
for at in range(0,len(points),8):
    lines.append('    '+','.join('{%d,%d}'%tuple(p) for p in points[at:at+8])+',')
lines+=['};','static const BearContour kBearContours[] = {']
for start,n,r,g,b in shapes:
    lines.append('    {%d,%d,rgb(%d,%d,%d)},'%(start,n,r,g,b))
lines+=['};','']
args.output.write_text('\n'.join(lines))
print(len(shapes),'contours;',len(points),'vertices; max',max(s[1] for s in shapes),'points;',args.output.stat().st_size,'bytes')
