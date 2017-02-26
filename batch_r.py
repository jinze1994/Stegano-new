#! /usr/bin/python
import os

srcPath = 'facebook-album-277565902678204'

for fname in os.listdir(srcPath):
    os.system("./naive --read \"%s\" password" % os.path.join(srcPath, fname))
