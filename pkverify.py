#!/usr/bin/python

import sys
import sha
import os
import struct

EXTRACTED_SIGFILE = 'extracted-signature.bin' # this is the input file to RsaUtlVerifyPriv 
DECRYPTED_DATAFILE = 'decrypted.bin'
SIGNATURE_PARTITION = '/dev/tffsd'
VERIFY_ERROR = -1
VERIFY_SUCCESS = 1
VERIFY_FAILED = 0

def strToHex(aString):
	hexlist = ["%02X" % ord(x) for x in aString]
	return ''.join(hexlist)

def HexTostr(hString):
	res = ""
	for i in range(len(hString)/2):
		realIdx = i*2
		res = res + chr(int(hString[realIdx:realIdx+2],16))
	return res

def RsaUtlVerifyPriv(inputfilename, keyfilename):
	""" Returns the verified and decrypted data """
	data = None
	cmdstr = "openssl rsautl -verify -in %s -out %s -inkey %s -pubin" % (inputfilename, DECRYPTED_DATAFILE, keyfilename)
	put, get = os.popen4(cmdstr)
	output = get.readlines()
	#print "RsaUtlSignPriv output: %s" %  output
	if output != []:
		return False
	else:
		try:
			sfile = open(DECRYPTED_DATAFILE, 'rb')
			data = sfile.read()
		except:
			return False
		return  data


def ExtractAndVerify(partition, pubkfile, verifiedDecodedBenyStr=[]):
	""" 
	Verifies that the beni string is in tact by extracting the signed digest, decrypting it and comparing it
	with a calculated sha1 digest of the uncrypted beni str

	returns:
		VERIFY_ERROR on error 
		VERIFY_SUCCESS when verification is successful, you should then provide 
			       a returnedBeniStr list that will hold the extracted uncrypted beni str that is safe to use (verified)
		VERIFY_FAILED when verification failed, the beni str has been tampered and thus we need to halt and not continue booting

	"""
	beni_str_unprotected = None
	calcd_digest = None
	signature = None
	EXTRACTED_TEMP_FILE = 'extracted-temp.bin'
	
	try:
		partifile = open(partition, 'rb')
	except:
		return VERIFY_ERROR
	# first, find the LNUX identifier that is supposed to be at 0x2e68
	partifile.seek(0x2e68)
	id = partifile.read(4)
	if id!="LNUX": return VERIFY_ERROR # error if we don't find our identifier
	try:
		packed_markers = partifile.read(8)
		markers_tuple = struct.unpack('LL',packed_markers)
	except:
		return VERIFY_ERROR
	# print markers_tuple
	beni_str_length = markers_tuple[0]
	signature_length = markers_tuple[1]
	try:
		beni_str_unprotected = partifile.read(beni_str_length)
		signature = partifile.read(signature_length)
		partifile.close()
	except:
		return VERIFY_ERROR

	try:
		extractf = open(EXTRACTED_SIGFILE, 'wb')
		extractf.write(signature)
		extractf.close()
	except:
		return VERIFY_ERROR
	decrypted_digest = RsaUtlVerifyPriv(EXTRACTED_SIGFILE, pubkfile)
	if not decrypted_digest:
		return VERIFY_ERROR
	decrypted_digest = strToHex(decrypted_digest)
	# print "decrypted_digest : %s" % decrypted_digest
	calcd_digest = sha.new(beni_str_unprotected)
	calcd_digest = calcd_digest.hexdigest().upper()
	# print "calcd_digest: %s " % calcd_digest
	if calcd_digest != decrypted_digest:
		return VERIFY_FAILED
	verifiedDecodedBenyStr.append(beni_str_unprotected)
	return VERIFY_SUCCESS


if __name__ == "__main__":
	beni_string = []
	public_key_file = None
	partition_name = None
	try:
		partition_name = sys.argv[1]
		public_key_file = sys.argv[2]

	except:
		print "Usage: %s [partition] [public_key_file]" % (sys.argv[0])
		sys.exit(-1)
	res = ExtractAndVerify(partition_name, public_key_file, beni_string)
	if res==VERIFY_SUCCESS:
		print "Success; decrypted string: %s " % strToHex(beni_string[0])
	if res==VERIFY_FAILED:
		print "Verification failed. We should now halt and not let the user continue boot process"
		sys.exit(VERIFY_FAILED)
	if res==VERIFY_ERROR:
		print "There has been an operational error trying to verify. has signature data been tampered with?? halting!"
	
	

