#!/usr/bin/python

import os
from spawndetach import SpawnDetach

EXEC_PROG = '/usr/lib/xpclient/AutoUpgrade'
UPGRADE_INI = '/root/lx_upgrade/2xxx/1.0.0/upgrade/xtremeupgrade.ini'


def DoUpgrade():
	if os.path.exists(UPGRADE_INI):
		SpawnDetach('/usr/lib/xpclient/eventsd')
		os.execl(EXEC_PROG)
	else:
		print 'No upgrade ini file found. skipping upgrade.'


if __name__ == "__main__":
	DoUpgrade()


	
