#!/usr/bin/env python3

from PIL import Image
import sys

threshold = 128

im = Image.open(sys.argv[1])
w,h = im.size

pix = im.load()

bmp = [[1 if pix[j,i][3]>threshold else 0 for j in range(w)] for i in range(h)]

def chunks(lst, n):
    """Yield successive n-sized chunks from lst."""
    for i in range(0, len(lst), n):
        yield lst[i:i + n]


for r in bmp:

    line = ', '.join([str(sum([b<<(7-i) for i,b in enumerate(bits)])) for bits in chunks(r, 8) ])
    print(' '.join(['.' if b==1 else ' ' for b in r]))

for r in bmp:

    line = ', '.join([str(sum([b<<(7-i) for i,b in enumerate(bits)])) for bits in chunks(r, 8) ])
    print("    {},".format(line))
