Linux Production Software v0.1
==============================

The linux production software is used to read the MAC address and CMA number from the stickers on
the unit itself, verify that the CMA is indeed correct and matches the hardware configuration, as well
as digitally signing the unit so on subsequent reboots it will boot as a ready desktop thinclient.

*Python* was instrumental in this simple yet deadly useful piece of software. This shows Python strengths
as *highly* suitable for low level system and hardware development, while maintaining its high level
readability making it easy to maintain and endow.

If you find this useful for your needs, please let me know. Feel free to contact me if you
might want to use this and need modification or tweaks for you environment.

Layout and Construction
=======================

The software is delivered on a USB disk image, basically containing a customized , trimmed size debian
distribution. I've removed all symlinks to unneccessary init scripts from /etc/rc?.d , made sure there are no traces
for a graphic system (no need for X and related components) and removed its unneccessary packages. So this
setup boots faster then a normal linux system, to minimize time loss at the production environment.

Almost everything related to the software is stored at /root. this includes intermediate files as well as core script
files that are executed part of the software. Exception to these are:

1. dummydll.dll file, used for creating the windoes style signature (the process was plainly ported from CE). 
   It is expected at : 
   /doc/cert/dummy/dummydll.dll 
   so if you're preparing a new USB image for production, make sure you copy this file from 
   svn://sivan/prodsoft/dummydll.dll to the mentioned location or the production software won't work.

2. /root/lx_upgrade/2xxx/1.0.0/upgrade/xtremeupgrade.ini should exist to trigger an image 
   upgrade after signing and registration happend through the production software. The production software 
   will find this file and execute the image upgrade tool to carry on the upgrade.
   
3. /usr/lib/xpclient/AutoUpgrade is upgrade tool binary the production software will try to execute. Make sure
   this binary is in fact available at that location and that it is executable. Consult Mark Lifshitz for newer
   version of this or any errors with it.

4. /usr/lib/xpclient/eventsd is crucial for the upgrade process and must be available from this location.
   The production software runs this program in the background. Consult Mark Lifshitz for any questions or
   issues regarding thie file.

Ths production software is written in Python, and uses the Urwid user interface library for its UI and interactions.
So, to run the software a python interpreter must be installed onto the image as well as the library:

$ apt-get install python2.5 (2.5 is the latest version as of this writing)

$ apt-get install python-urwid (for the Urwid library, see https://excess.org/urwid/wiki/Documentation for more details)


Digital Signature Process
=========================

The linux production software uses openssl's rsa utility to actually create a digital signature on the
/dev/tffsd partition. This is later to be verified by the desktop image that boots after production cycle is finished.
In order for the signing process to take place, the usb disk containing the production software image must also contain
the production secret key. I created the key pair using:

$ openssl genrsa -out private.key [bit-size] (you can use bit size from 1024 up to 2048 if you're creating a new pair)

This creates a key pair , basically holding both the public and the private key inside. We need the public key seperate so
we will extract it from the private.key file we just created:

$ openssl rsa -in private.key -out public.key -outform PEM -pubout

This creates the public.key file which contains the public key portion and can be used by pkverify.py to verify
that a signature is intact and untouched on the /dev/tffsd partition.

The key pair which is currently used by the production software is available from svn. To access it
go to svn://compi/sivan/prodsoft/keys. There are two files there, one for public and the other for private key.

The private.key file should be copied onto the /root folder at the production software image so it could be found
by the openssl utility. In turn, the openssl utility must be installed onto the production image by issuing:

$ apt-get install openssl

The package is available from our soft-float repository. Consult svn://compi/sivan/floatfind/trunk/README.txt in the
section "Using The Repository" for instructions how to actually set up an image or a client to fetch soft-float enabled
packages.


To fetch a ready made image of the production software from svn, go to svn://compi/sivan/prodsoft/production_image . This
is a complete ready made image you can copy right away to a usb disk and use on production line. 

