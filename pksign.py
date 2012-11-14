#!/usr/bin/python

import sys
import sha
import os
import struct

TEMPFILE_CLEAR = 'before-sign.bin'
TEMPFILE_SIGNED = 'after-sign.bin'
TEMPFILE_SIGNED_FINAL = 'signed-final.bin'

def RsaUtlSignPriv(inputfilename, outputfilename, keyfilename):
	data = None
	cmdstr = "openssl rsautl -in %s -out %s -inkey %s -sign" % (inputfilename, outputfilename, keyfilename)
	put, get = os.popen4(cmdstr)
	output = get.readlines()
	#print "RsaUtlSignPriv output: %s" %  output
	if output != []:
		return False
	else:
		try:
			sfile = open(TEMPFILE_SIGNED, 'rb')
			data = sfile.read()
		except:
			return False
		return  data


def CreateSignedFile(bstr, privkfile):
	bstr_bin = bstr.decode('hex')
	s = sha.new(bstr_bin)
	sha_string = s.hexdigest()
	sha_string_bin = sha_string.decode('hex')
	try:
		tosignfile = open(TEMPFILE_CLEAR,'wb')
	except:
		return False
	tosignfile.write(sha_string_bin) # writing a binary form of the digest to a file
	tosignfile.close()
	# create the openssl signature using the private key.
	signed_res = RsaUtlSignPriv(TEMPFILE_CLEAR, TEMPFILE_SIGNED, privkfile)

	if not signed_res: # return in an error if signature was not created
		return False
		
	# create the markers
	#print "\n"
	#print "%s size:%s" % (TEMPFILE_SIGNED,  os.path.getsize(TEMPFILE_SIGNED))
	#print "signed_res size:", len(signed_res)
	#print "bstr_bin size:", len(bstr_bin)
	#print "\n"

	markers = struct.pack('LL',len(bstr_bin),len(signed_res))

	# now add the markers, which include the size of the beni str, size of the digested beni str, size of signed beni str
	# then we have to add to the file the clear bin repr of beni str, the digested beni str before the signature which is already
	# contained in the file. so we take the signed_res which holds the signed data and write lastly to the file
	try:
		fsigned = file(TEMPFILE_SIGNED_FINAL, 'wb')
		fsigned.write('LNUX')
		fsigned.write(markers)
		fsigned.write(bstr_bin)
		fsigned.write(signed_res)
		fsigned.close()
	except:
		return False

	return signed_res

	


if __name__ == "__main__":
	beni_string = None
	private_key_file = None
	try:
		beni_string = sys.argv[1]
		private_key_file = sys.argv[2]

	except:
		print "Usage: %s [beni_string] [private_key_file]" % (sys.argv[0])
		sys.exit(-1)
	signature_string = CreateSignedFile(beni_string, private_key_file)
	if not signature_string:
		print "Creating signature string failed."
	else:
		print signature_string
	
	

