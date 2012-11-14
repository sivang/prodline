#!/usr/bin/env python
# Copyright (C) 2005-2006 Sivan Greenberg. <sivan@ubuntu.com>
# 
# This software is distributed under the terms and conditions of the GNU General
# Public License. See http://www.gnu.org/copyleft/gpl.html for details.

import os

DEBUG_MODE = True

def ensure_path(create, path):
    comps = path.split("/")
    comps = [i for i in comps if i!='']
    filepart = None
    root = ""
    if os.path.isfile(path):
            filepart = comps[-1]
            comps.remove(filepart)
    # print comps
    for part in comps:
        cur = root + "/" + part
        if not os.path.exists(cur): 
            if create: 
                os.mkdir(cur)
            if DEBUG_MODE: print "creating directory %s" % (cur)
        root = root + "/" + part


