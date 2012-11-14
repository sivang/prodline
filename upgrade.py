#!/usr/bin/python

import os
from spawndetach import SpawnDetach

EXEC_PROG = '/usr/lib/xpclient/AutoUpgrade'


def DoUpgrade():
	SpawnDetach('/usr/lib/xpclient/eventsd')
	os.execl(EXEC_PROG)


if __name__ == "__main__":
	DoUpgrade()


	
