#!/usr/bin/python
import sys
import os

def SpawnDetach(path):
	os.spawnl(os.P_NOWAIT, path)


if __name__ == "__main__":
	SpawnDetach(sys.argv[1])


	
