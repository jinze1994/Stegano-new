#! /usr/bin/python
import os

srcPath = 'facebook'

for fname in os.listdir(srcPath):
    os.system("./naive --read \"%s\" password" % os.path.join(srcPath, fname))
