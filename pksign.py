#!/usr/bin/python

import sys
import sha
from mytest2 import FileData

def CreateSignatureString(bstr, privkfile):
	tmp1 = FileData(privkfile)
	tmp2 = tmp1[:-1]
	tmp3 = tmp2[1:]
	privkdata = "".join(tmp3)
	print privkdata
	s = sha.new(bstr)
	sha_string = s.hexdigest()
	print sha_string
	# print sha_string.decode('hex') # this is to print a binary representation good for writing to a file and then to the partition
	


if __name__ == "__main__":
	beni_string = None
	private_key_file = None
	try:
		beni_string = sys.argv[1]
		private_key_file = sys.argv[2]

	except:
		print "Usage: %s [beni_string] [private_key_file]" % (sys.argv[0])
		sys.exit(-1)
	signature_string = CreateSignatureString(beni_string, private_key_file)
	if not signature_string:
		print "Creating signature string failed."
	else:
		print signature_string
	
	

