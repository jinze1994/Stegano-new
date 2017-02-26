#! /usr/bin/python
import os
from PIL import Image

srcPath = 'test'
dstPath = 'out'

for fname in os.listdir(srcPath):
    img = Image.open(os.path.join(srcPath, fname))
    img = img.resize((400, 720))
    img.save('tmp.jpg')
    img.close()
    os.system("./naive --write tmp.jpg \"%s\" password https://1drv.ms/u/s!Am4Udwu9NRRagQjj4kLBGyUtYaEl" % os.path.join(dstPath, fname))
