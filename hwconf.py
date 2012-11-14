#!/usr/bin/python

import mytest2

LX_HDMI = 0
LX_DVI = 1
EFI_L_DVI_I = 2
NG_L_DVI_VGA = 3
NG_L_DVI_VGA_Wireless = 4


CMA_READ_ERROR = -1

CMA_TABLE = {
		'010000111' : 'CMA03995',
		'100000111' : 'CMA03995',
		'110000111' : 'CMA03997',
		'110000011' : 'CMA04003',
		'110000011' : 'CMA04163',
		'100001010' : 'CMA04165',
		'110011010' : 'CMA04050',
		'110011010' : 'CMA04166',
		'100101101' : 'CMA04094',
		'110101101' : 'CMA04164',
		'100111111' : 'UNDEFINED' }



def bin(n):
	digits = {'0':'000','1':'001','2':'010','3':'011',
		  '4':'100','5':'101','6':'110','7':'111'}
	octStr = "%o" % n
	binStr = ''
	# convert octal digit to its binary equivalent
	for c in octStr: binStr += digits[c]
	return binStr

def returnSearchableBitString(binstr):
	tempStr = ""
	tempStr = binstr[6] + binstr[7] + binstr[4] + binstr[5] + binstr[2] + binstr[3] + binstr[13] + binstr[12] + binstr[9]
	return tempStr

def returnCPUSpeed(binstr):
	if binstr[6]=="0":
		if binstr[7]=="0":
			return 396
		else:
			return 528
	if binstr[6]=="1":
		if binstr[7]=="0":
			return 600
		else:
			return 696

def returnRAMSize(binstr):
	if binstr[4]=="0":
		if binstr[5]=="0":
			return 128
		else:
			return 256

def returnProductConfig(binstr):
	if binstr[2]=="0":
		if binstr[3]=="0":
			if binstr[13]=="0":
				return LX_HDMI
			else:
				return LX_DVI
		else:
			# binstr[3]=="1"
			if binstr[13]=="0":
				return EFI_L_DVI_I
			else:
				return NG_L_DVI_VGA
	else:
		# binstr[2]=="1"
		return NG_L_DVI_VGA_Wireless

def hasSmartCard(binstr):
	if binstr[12]=="0":
		return True
	else:
		return False

def hasPoe(binstr):
	if binstr[9]=="0":
		return True
	else:
		return False
			

def returnHwConfig():
	RAMSize = None
	CPUSpeed = None
	ProductConfig = None
	ScConfig = None
	poeConfig = None
	FlashSize = None
	FlashSize = mytest2.FileData('/proc/chippc/flash_size')
	if not FlashSize: return None
	FlashSize = int(FlashSize[0])
	value = mytest2.FileData('/proc/chippc/product_config')
	if not value: return None
	value = returnBitString(value[0])
	binstr = reverseBitString(value)
	CPUSpeed = returnCPUSpeed(binstr)
	RAMSize = returnRAMSize(binstr)
	ProductConfig = returnProductConfig(binstr)
	ScConfig = hasSmartCard(binstr)
	poeConfig = hasPoe(binstr)
	return (CPUSpeed, FlashSize, RAMSize, ProductConfig, ScConfig, poeConfig)


def returnBitString(hex_value):
	if type(hex_value) == type('str'):
		value = int(hex_value,16)
	else:
		value = hex_value
	return convbin(value)

def reverseBitString(binstr):
	clist = []
	for c in binstr:
		clist.append(c)
	clist.reverse()
	return ''.join(clist)
	
def returnCMA(bitstring):
	for_cma_bitstring = returnSearchableBitString(reverseBitString(bitstring))
	try:
		temp = CMA_TABLE[for_cma_bitstring]
	except:
		return 'NOT_IN_TABLE'
	return temp
		
def returnBoardCMA():
	hexvalue_in_list = mytest2.FileData('/proc/chippc/product_config')
	hexvalue_as_string = hexvalue_in_list[0]
	if not hexvalue_as_string: return None
	bitstring = returnBitString(hexvalue_as_string)
	return returnCMA(bitstring)

def cmaIsCorrect(cma_string):
	cma_string = 'CMA' + cma_string
	reportedCMA = returnBoardCMA()
	if not reportedCMA:
		return CMA_READ_ERROR
	if reportedCMA != cma_string:
		return False
	else:
		return True

def returnHwConfigHexa():
	hexvalue_in_list = mytest2.FileData('/proc/chippc/product_config')
	hexvalue_as_string = hexvalue_in_list[0]
	if not hexvalue_as_string: return None
	return hexvalue_as_string

class verifyCmaByHttpRequest:
	def __init__(self, cma_string, field_name='cma_string'):
		postURL = mytest2.FileData('posturl-cma.cfg')[0]
		self.success = None
		self.error = None
		self.cma_match = False
		url = postURL
		params = {field_name : cma_string}
		data = urllib.urlencode(params)
		try:
			response = urllib.urlopen(url, data, proxies={})
		except IOError, e:
			self.error = "Can't get data from server: %s" % e[1]
			self.error += "\nCheck that link is up and that the server is online and try again."
			self.success = False
		for line in response:
			line = line.strip()
			if line.find('ServerError') > -1:
				self.error = 'Server Responded ' + line.split(':')[1]
				self.success = False
				return
			if line.find('MATCH') > -1:
				self.error = None
				self.success = True
				return
			if line.find('NOMATCH') > -1:
				self.error = 'CMA Was not verified. Please check that you are reading the right sticker and try again.'
				self.success = False
				return
			if line=='':
				self.error = 'Empty response from server. Please check that production server is online and setup correctly and try again.'
				self.success = False


def tobase(base,number):
    global tb
    def tb(b,n,result=''):
        if n == 0: return result
        else: return tb(b,n/b,str(n%b)+result)

    if type(base) != type(1):
        raise TypeError, 'invalid base for tobase()'
    if base <= 0:
        raise ValueError, 'invalid base for tobase(): %s' % base
    if type(number) != type(1) and type(number) != type(1L):
        raise TypeError, 'tobase() of non-integer'
    if number == 0:
        return '0'
    if number > 0:
        return tb(base, number)
    if number < 0:
        return '-' + tb(base, -1*number)

convbin = lambda n: tobase(2,n)


if __name__ == "__main__":
	res = returnHwConfig()
	if res:
		print "Hardware Configuration:"
		print res
		print "Board CMA"
		print returnBoardCMA()
	else:
		print "Failed acquiring hardware configuration of unit"
