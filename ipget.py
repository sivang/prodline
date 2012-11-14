#!/usr/bin/python

import socket
import fcntl
import struct
import sys

def get_ip_address(ifname):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    return socket.inet_ntoa(fcntl.ioctl(s.fileno(),0x8915,struct.pack('256s', ifname[:15]))[20:24])

def wait_ip_address(ifname, func):
	ip_addr = None
	while not ip_addr:
		func()		
		try:
			ip_addr = get_ip_address(ifname)
		except:
			ip_addr = None
	return ip_addr


if __name__ == '__main__':
	print "Waiting for ip address"
	myip = wait_ip_address(sys.argv[1])
	print "Got ip address", myip
