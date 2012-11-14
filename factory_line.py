#!/usr/bin/python
import urwid.curses_display
import urwid
import time
import os
import urllib
import chinsig
from ipget import *
from upgrade import DoUpgrade

postURL = None 	# url against which we post the hwconfig and expect to receive signatures
		# currently the value is being read from a file named posturl.cfg at the same dir where this runs


class FileData(list):
	def __init__(self, proc_entry):
		try:
			f = file(proc_entry)
			content = f.readlines()
		except:
			return None
		content = [line.strip() for line in content]
		content = [line for line in content if line!='\n']
		content = [line for line in content if line!='' and len(line)>0]
		self.extend(content)

			


def isHexD(digit):
	numbers = range(10)
	numstrs = []
	for n in numbers:
		numstrs.append(n.__str__())
	alphas = ['A','B','C','D','E','F']

	if digit in numstrs or digit.capitalize() in alphas:
		return True
	else:
		return False

def validateMac(str):
	comps = str.split(":")
	for i in comps:
		i.strip()
	recon = "".join(comps)
	if len(recon) != 12:
		return False
	for d in recon:
		if isHexD(d):
			pass
		else:
			return False
	return True

def validateCma(str):
	if len(str) != 5:
		return False
	if not str.isdigit():
		return False
	else:
		return True


def writeMAC(mac_addr):
	cmdstr = "./tffs_ioctl --write-mac %s" % mac_addr
	put, get = os.popen4(cmdstr)
	output = get.readlines()
	for line in output:
		if line.find('MAC written successfully') > -1:
			return None
	return output

def readMAC():
	cmdstr = "./tffs_ioctl --read-mac"
	put, get = os.popen4(cmdstr)
	output = get.readlines()
	for line in output:
		if line.find("MAC address is:") > -1:
			data = line.split(":")
			mac = data[1].strip()
			mac = mac.replace("'","")
			return mac
		else:
			return None

	print output

def readDocUniqueID():
	cmdstr = "./tffs_ioctl -u"
	put, get = os.popen4(cmdstr)
	output = get.readlines()
#	for line in output: DISABLE DEBUG PRINTS
#		print line
	for line in output:
		if line.find("DOC Unique ID is :") > -1:
			data = line.split(":")
			id = data[1].strip()
			return id
		else:
			return None
	
class getSigByHttpRequest(object):
	def __init__(self, hwconfig, field_name='hwconfig'):
		self.success = None
		self.error = None
		self.sigs = []
		url = postURL
		params = {field_name : hwconfig}
		data = urllib.urlencode(params)
		try:
			response = urllib.urlopen(url, data, proxies={})
		except IOError, e:
			self.error = "Can't get data from server: %s" % e[1]
			self.error += "\nCheck that link is up and that the server is online and try again."
			self.success = False
			return
		for line in response:
			line = line.strip()
			if line.find('ServerError')>-1:
				self.error = 'Server responded ' + line.split(":")[1]
				self.success = False
				return
			if len(line)>1:
				try:
					rawsig = line.split(":")[1]
					sig = rawsig.strip()
				except:
					self.error = "Cannot parse signatures from server, check that server response is correct and try again."
					self.success = False
					return
				self.sigs.append(sig)
		self.success = True
		self.error = None

def GetLocalIP():
	return get_ip_address('eth0')

def prepareHardwareConfigString(cma,mac,unique_id):
	LocalIP = None
	try:
		LocalIP = GetLocalIP()
	except:
		return None

	# get required data from proc entries
	my_ram = FileData('/proc/chippc/ram_size')
	my_flash = FileData('/proc/chippc/flash_size')
	my_cpuspeed = FileData('/proc/chippc/cpu_speed')
	my_bus = FileData('/proc/chippc/bus_speed')

	if not my_ram or not my_flash or not my_cpuspeed or not my_bus:
		return None
	MAC = mac
	CMA = cma
	StationMAC = MAC
	RAMonBoard = my_ram[0] # 128 if cannot be read from our /proc entries
	FlashOnBoard = my_flash[0] # always 256 if cannot be read by linux tools
	SerialPorts = 0
	ParallelPorts1 = 0
	SerialPorts2 = 0
	USBPorts = 4
	AudioIn = 1
	AudioOut = 1
	CPUType = my_cpuspeed[0] # if cannot be read from our /proc entries
	DisplayType = 2
	BusSpeed = my_bus[0]
	WirelessType = 0
	SmartCardType = 0
	ModemType = 0
	SubboardType = 0
	DebugBoard = 0
	DebugMode = 0
	LCD  = 0
	Current = 0
	Supply5V = 0
	Supply3V = 0
	Corevdd = 0
	UniqueID = unique_id
	format_str = "%s,%s,%s,%s,%s,%d,%d,%d,%d,%d,%d,%s,%d,%s,%d,%d,%d,%d,%d,%d,%d,%2.3f A,%2.3f V,%2.3f V,%2.3f V,%s,%s"
	result_str = format_str % (MAC,
					CMA,
					StationMAC,
					RAMonBoard,
					FlashOnBoard,
					SerialPorts,
					ParallelPorts1,
					SerialPorts2,
					USBPorts,
					AudioIn,
					AudioOut,
					CPUType,
					DisplayType,
					BusSpeed,
					WirelessType,
					SmartCardType,
					ModemType,
					SubboardType,
					DebugBoard,
					DebugMode,
					LCD,
					Current,
					Supply5V,
					Supply3V,
					Corevdd,
					UniqueID,
					LocalIP)
	return result_str



	
	

class Conversation(object):
	MAC_ADDR = None
	CMA_VALUE = None
	def __init__(self):
		self.items = urwid.SimpleListWalker(self.work_flowCMA())            #        comment when inputting not inputting CMA and replace with a workflow_MAC   line
		self.listbox = urwid.ListBox(self.items)
		instruct = urwid.Text("Linux Production Software v1.0b", 'center')
		statusline = urwid.Text("READY", 'center')
		self.status = statusline
		header = urwid.AttrWrap( instruct, 'header' )
		footer = urwid.AttrWrap( statusline, 'header')
		self.foot = footer
		self.top = urwid.Frame(self.listbox, header, footer)

	def main(self):
		self.ui = urwid.curses_display.Screen()
		self.ui.register_palette([
			('regular', 'black', 'dark cyan', 'standout'),
			('okay','white','dark green','standout'),
			('header', 'yellow', 'dark blue', 'bold'),
			('error', 'white', 'dark red', 'standout'),
			('field_name','white','black','standout')
			])
		self.ui.run_wrapper( self.run )
	
	def run(self):
		finished = False
		need_to_validate_CMA = True # set to false and change the init line
		size = self.ui.get_cols_rows()
		# self.CMA_VALUE = '12345' # use default dummy value since we do not input CMA in the time being
		self.CMA_VALUE = None # default before inputting CMA 
		ipaddress = False
		self.status.set_text('Waiting to obtain IP address...')
		while True:
			self.draw_screen( size )
			if not ipaddress: # make sure we have an ip address when we bring up UI
				ipaddress = wait_ip_address('eth0', self.ui.get_input)
				self.status.set_text("READY")
			keys = self.ui.get_input()
			if finished:
				break
			if keys:
				self.status.set_text("READY")
				self.foot.set_attr('header')
			for k in keys:
				if k == "window resize":
					size = self.ui.get_cols_rows()
					continue
				if need_to_validate_CMA:
					if k.isdigit() or k == 'backspace':
						self.top.keypress( size, k)
					if validateCma(self.editCMA.get_edit_text()):
						self.CMA_VALUE = self.editCMA.get_edit_text()
						self.items.extend(self.work_flowMAC())
						self.items.set_focus(3)
						need_to_validate_CMA = False
				elif isHexD(k) or k=='backspace': # we need to validate a MAC address
					self.top.keypress( size, k ) 
					if validateMac(self.editMAC.get_edit_text()):
						mac_addr = self.editMAC.get_edit_text()
						if validateMac(mac_addr):
							self.status.set_text("Writing MAC Address")
							self.MAC_ADDR = mac_addr
							res = writeMAC(self.MAC_ADDR) # returns None on success, output if error
							if res: 
								self.foot.set_attr('error')
								self.status.set_text("tffs_ioctl error trying to  write MAC address: " + "".join(res))
								continue
							else:
								id = readDocUniqueID()
								if id:
									self.foot.set_text('\n Contacting Server...\n')
									self.draw_screen( size )
									hwconfig = prepareHardwareConfigString(self.CMA_VALUE,self.MAC_ADDR,id)
									if not hwconfig:
										self.foot.set_attr('error')
										self.foot.set_text('Failed to get IP address. Check that the network interface is configured correctly and that it was assigned an IP address, restart the system and try again.')
										continue
									httpsig = getSigByHttpRequest(hwconfig) # create a "chineese" signature object
									if not httpsig.success:
										self.foot.set_attr('error')
										self.foot.set_text('Error ' + httpsig.error)
										continue
									else:
										sigcp = chinsig.SignatureCopier(chinsig.SIGNFILES,httpsig.sigs)
										if not sigcp.copy():
											self.foot.set_attr('error')
											self.status.set_text(sigcp.error)
											continue
										else:
											httpsig = getSigByHttpRequest(hwconfig,'sign_ok')
											if not httpsig.success:
												self.foot.set_attr('error')
												self.foot.set_text('Error ' + httpsig.error)
												continue
											self.foot.set_attr('okay')
											self.foot.set_text('\n O K \n')
											finished = True
											continue
								else:
									self.foot.set_attr('error')
									self.status.set_text("tffs_ioctl : cannot open device /dev/tffsa to read unique id.")
									continue
						else:
							self.foot.set_attr('error')
							self.status.set_text("Illeagle MAC address entered.")
							continue
					
					
	
	def draw_screen(self, size):
		canvas = self.top.render( size, focus=True )
		self.ui.draw_screen( size, canvas )

	def work_flowCMA(self):
		self.editCMA = urwid.Edit(align='center')
		return [urwid.Text(('field_name',"Enter CMA:"),'center'),
			urwid.LineBox(urwid.AttrWrap(self.editCMA,'regular'))]

	def work_flowMAC(self):
		self.editMAC = urwid.Edit(align='center')
		return [urwid.Text(('field_name',"Enter MAC:"), 'center'),
			urwid.LineBox(urwid.AttrWrap(self.editMAC,'regular'))]

	def new_answer(self, name):
		return urwid.Text(('I say',"Nice to meet you, "+name+"\n"))
	
	def getMac(self):
		return self.MAC_ADDR


if __name__=="__main__":
	url_from_file = FileData('posturl.cfg')
	postURL = url_from_file[0]
	gui = Conversation()
	gui.main()
	DoUpgrade()

