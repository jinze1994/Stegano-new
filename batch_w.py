#! /usr/bin/python
import os
from PIL import Image

srcPath = 'origin'
dstPath = 'out'

for fname in os.listdir(srcPath):
    os.system("./naive --write \"%s\" \"%s\" password https://1drv.ms/u/s!Am4Udwu9NRRagQjj4kLBGyUtYaEl" % (os.path.join(srcPath, fname), os.path.join(dstPath, fname)))
