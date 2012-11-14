#!/usr/bin/python

import os

def DoUpgrade():
	os.execl('/usr/lib/xpclient/AutoUpgrade','')



if __name__ == "__main__":
	print "This is spawnutil"
