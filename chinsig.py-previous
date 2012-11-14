#!/usr/bin/python

import sys
import shutil
import os
from struct import *
import array
import fsMisc

CERT_DIR = "/doc/cert" 
CERT_DLL_PATH = "/dummy"
CERT_DLL = CERT_DIR + CERT_DLL_PATH +  "/dummydll.dll"
CERT_SYST_ERROR = -2
CERT_LOCATE_ERROR = -6
CERT_LENGTH = 80
CERT_SUCCESS   = 1
SIGNFILES = ["signed1.bin","signed2.bin","signed3.bin","signed4.bin"]



def LocateCert(myfile, CertSize=None): # set CertSize=[] to receive back the size of the Cert
	"""
	Takes an open file object and an optional empty list
	and returns the location inside the file where the cert resides , as well
	as the cert size as [size] so use CertSize[0] to get the value.
	"""
	wPE32basedValue = 0 # if wPE32 == 0x10b (267) , then value is 128 otherwise 144
	dw = 0
	dwSize = 0
	wPE32 = None
	myfile.seek(0x3c,0)
	dw = myfile.read(4)
	# print "dw = %s" % unpack('<i',dw)
	myfile.seek(unpack('<i',dw)[0] + 0x18)
	wPE32 = myfile.read(2)
	# print "wPE32 = %s" % unpack('<h',wPE32)
	if unpack('<h',wPE32)[0]==267:
		wPE32basedValue = 128
	else:
		wPE32basedValue = 144
        # print "wPE32basedValue = %s" % wPE32basedValue
	myfile.seek(unpack('<i',dw)[0] + 24 + wPE32basedValue,0)
	dw = myfile.read(4)
	dw_value = unpack('<I',dw)[0]
	if not dw_value:
		return 0
	dwSize = myfile.read(4) # supposedly I should wrap this in an exception for when it reaches beyond EOF
	# print "dwSize = %s" % unpack('<i',dwSize)
	if CertSize==[]:
		myfile.seek(unpack('<i',dw)[0] + 8,0)
		certsize = myfile.read(4)
		# print "CertSize = %s" % unpack('<I',certsize)
		CertSize.append(unpack('<i',certsize)[0])
	myfile.seek(unpack('<i',dw)[0] + 12,0)
	return (unpack('<i',dwSize)[0] - 12)

def WriteCert(tofilename, cert): # expects cert to already be a string of hexadecimals pairs each representing a byte
	# print "CERT_DIR = ",CERT_DIR
	# print "CERT_DLL = ",CERT_DLL
	f = None # file "handle"
	cert_byte = cert.decode('hex')
	fsMisc.ensure_path(True,CERT_DIR)
	# print "tofilename = ",tofilename
	# print "cert = ", cert
	shutil.copy(CERT_DLL, tofilename)
	try:
		f = open(tofilename,'r+b')
	except:
		# print "Faild to open %s" % tofilename
		return CERT_SYST_ERROR
	if not LocateCert(f,None):
		return CERT_LOCATE_ERROR
	# print "f's position: ",f.tell()
	f.write(cert_byte)
	f.close()
	return CERT_SUCCESS

def AddFileHeader(szFileName):
	hFile = None
	resFile = None
	orig_filecontent = None
	CERT_MAGIC = 0x53452544
	try:
		hFile = file(szFileName,'rwb')
		resFile = file(szFileName+".tmp",'wb')
	except:
		return False
	hFile.seek(0,2)
	FILE_SIZE = hFile.tell() # store current file pointer which is actually the file's size
	hFile.seek(0,0) # put file pointer to start of file
	buffer = array.array('L') # prepare a buffer  (array) that holds DWORDs
	# prepare the header data into the array
	buffer.append(CERT_MAGIC)
	buffer.append(FILE_SIZE)
	# now write header infomation to file
	buffer.tofile(resFile)
	resFile.close()
	try:
		resFile = file(szFileName+".tmp",'ab') # open resFile for append, to add the file content we will read from the original file
		orig_filecontent = hFile.read(FILE_SIZE)
		resFile.write(orig_filecontent)
		resFile.close()
		hFile.close()
	except:
		return False
	os.remove(szFileName) # remove the original file (without header)
	shutil.copy(szFileName+".tmp",szFileName) # copy the new file (.tmp) with the header to the original file name
	os.remove(szFileName+".tmp") # remove the tmp file since it is no longer needed
	return True

def SignFile(szResultFile, lpSignature):
	res = None
	res = WriteCert( szResultFile, lpSignature)
	if res != CERT_SUCCESS:
		return False
	return True

def SignFiles(FileNames, Signatures):
	signature = None
	filename = None
	Signatures.reverse()
	for filename in FileNames:
		try:
			signature =  Signatures.pop()
		except IndexError:
			return True
		if not SignFile(filename, signature):
			print "SignFile failed!"
			return False
		if not AddFileHeader(filename):
			print "AddFileHeader failed!"
			return False
	return True

def AlignFileTo512(f): # f is assumed to be an open file for append
	filesize = f.tell()
	filesize = (filesize + 511) & (-512L) # calculate new larger file size by rounding up to the next multiple of 512
	try:
		f.seek(filesize)
		f.truncate()
	except:
		print "AlignFileTo512 failed!"
		return False
	return True


def AppendFiles(FileNames):
	fnames = FileNames[:]
	bRes = False
	fnames.reverse()
	hfile = None
	mainfilename = fnames.pop()
	# print "mainfilename = ", mainfilename
	try:
		mainfile = file(mainfilename,'r+b') #  open for append to a binary file to round up its size to the next 512 multiple
	except:
		return False
	fnames.reverse()
	for filename in fnames:
		if not AlignFileTo512(mainfile):
			return False
		try:
			fl = file(filename, 'rb')
		except:
			return False
		shutil.copyfileobj(fl, mainfile)
		fl.close()
	mainfile.close()
	return True


def WriteFileToPartition(filename, partition):
	# we need to copy filename to /dev/tffsd
	try:	
		# print "Trying to copy %s to /dev/tffsd" % filename
		filesize = os.path.getsize(filename)
		source = file(filename,'rb')
		target = file(partition,'wb')
		shutil.copyfileobj(source, target)
		source.close()
		target.close()
	except:
		return False
	return True
	

class SignatureCopier:
	def __init__(self, filenames=[], signatures=[]):
		self.filenames = filenames
		self.signatures = signatures
		self.success = None
		self.error = None
	def copy(self):
		res = SignFiles(self.filenames, self.signatures)
		if not res:
			self.success = False
			self.error = 'Error while signing files! exitting.'
			return False
		res = AppendFiles(self.filenames)
		if not res:
			self.success = False
			self.error = 'Error while appending files! exitting.'
			return False
		res = WriteFileToPartition(self.filenames[0],'/dev/tffsd')
		if not res:
			self.success = False
			self.error = 'Failed writing file to partition! exitting.'
			return False
		return True

if __name__=="__main__":
	""" this is the unit test, takes one signature and applies it to all files """
	onesig = "66666666666666666232666666666666316533633066623030373030303938363285F9D30C83820293E640854E1BF87F0405EC5C5DBE5E2683FCB03FA16B62741A289F0ED934313EF2236163BFAB5832"
	sigs = [onesig, onesig, onesig, onesig]
	sigcp = SignatureCopier(filenames=SIGNFILES, signatures=sigs)
	sigcp.copy()
	if sigcp.error:
		print "* Problem: %s" % sigcp.error
	else:
		print "* Signature copied successfuly."
