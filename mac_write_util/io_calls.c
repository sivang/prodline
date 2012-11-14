#define _IN_APP_
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "tffsioct.h"
#include "Xat/xatioctl.h"
#include <errno.h>
#include <err.h>
#include <unistd.h>
#include <string.h>
#include <glib.h>

int dismount_dev_fd(int fd,FLStatus* pstat)
{
	flIOctlRecord        ioctl_data;
	flMountInput in;
	flOutputStatusRecord out;
	int res = 0;
	in.type = FL_DISMOUNT;
    ioctl_data.inputRecord  = &in;
    ioctl_data.outputRecord = &out;
    res = ioctl (fd, FL_IOCTL_MOUNT_VOLUME, &ioctl_data);
    *pstat = out.status;
    return res;
}

int dismount_dev(char* dev_name)
{	
	int res = 0;
	FLStatus status = flOK;
	int fd = open(dev_name,O_RDWR);
	if(-1 == fd)
	{
		res = errno;		
		fprintf(stderr, "Cannot open device '%s'. %s", dev_name,strerror(res));
		goto End;
	}
	
    res = dismount_dev_fd(fd,&status) ;
    if ((res != 0) || (status != flOK))
    {
        fprintf (stderr, "FL_IOCTL_MOUNT_VOLUME/FL_DISMOUNT operation failed for device '%s'.\n"
        				 "ioctl returned %d, TFFS status is %d\n",
        		dev_name,res, status);
        res = -1;        
    }
End:
	if(-1 != fd)
	{
		close(fd);
	}
	return res ; 
}

int fl_ioctl_ipl(int fd, char* buf, size_t buf_size)
{
	flIOctlRecord        ioctl_data;
	flIPLInput			in;
    flOutputStatusRecord out;
    int                  rc;
    bzero(&in,sizeof(in));
    bzero(&out,sizeof(out));
    
    in.bBufPtr = (unsigned char*)buf;
    in.sdwLength = buf_size;
    in.dwFlags = FL_IPL_MODE_NORMAL;
    in.sdwOffset = 0;
    
    ioctl_data.inputRecord  = &in;
    ioctl_data.outputRecord = &out;
    
    rc = ioctl (fd, FL_IOCTL_WRITE_IPL, &ioctl_data);

    if ((rc != 0) || (out.status != flOK))
    {
    	if(0 == rc)
    	{
    		rc = -1 ;
    	}
        fprintf (stderr, "FL_IOCTL_WRITE_IPL failed %d\n", out.status);
    }
    return rc;
}

int fl_ioctl_read_unique(int fd,unsigned char *buffer16)
{
	flIOctlRecord       ioctl_data;
	flUniqueIdOutput	out;
	int                  rc;
	
	ioctl_data.inputRecord = NULL;
	ioctl_data.outputRecord = &out;
	
	rc = ioctl (fd, FL_IOCTL_UNIQUE_ID, &ioctl_data);
	
	if ((rc != 0) || (out.status != flOK))
    {
    	if(0 == rc)
    	{
    		rc = -1 ;
    	}
        fprintf (stderr, "FL_IOCTL_UNIQUE_ID failed with DOC status %d\n", out.status);
    }
	else
	{
		memcpy(buffer16,out.id,sizeof(out.id));
	}
	return rc;	
}

int fl_ioctl_flash_format (int fd, int nPartitions,BDTLPartitionFormatParams3  partitions[])
{
    flIOctlRecord        ioctl_data;
    flOutputStatusRecord out;
    int                  rc;

     /* format DiskOnChip */
    {   flFlashFormatInput           in;
        FormatParams3                fp3;
        
        memset (&in,   0, sizeof(in)); 
        memset (&fp3,  0, sizeof(FormatParams3));

        in.fpPtr         = &fp3;
        in.dwFormatFlags = TL_NORMAL_FORMAT;                
        fp3.BDTLPartitionInfo    = &partitions[0];                
        fp3.noOfBDTLPartitions   = nPartitions ;

        ioctl_data.inputRecord  = &in;
        ioctl_data.outputRecord = &out;

        rc = ioctl (fd, FL_IOCTL_FLASH_FORMAT, &ioctl_data);

        if ((rc != 0) || (out.status != flOK))
        {
        	if(0 == rc)
	    	{
	    		rc = -1 ;
	    	}
            fprintf (stderr, "FL_IOCTL_FLASH_FORMAT failed %d\n", out.status);
        }
    }

    /* re-mount disk */
    {   flMountInput in;

        in.type = FL_MOUNT;
        ioctl_data.inputRecord  = &in;
        ioctl_data.outputRecord = &out;
        rc = ioctl (fd, FL_IOCTL_MOUNT_VOLUME, &ioctl_data);
        if ((rc != 0) || (out.status != flOK))
        {
        	if(0 == rc)
	    	{
	    		rc = -1 ;
	    	}
            fprintf (stderr, "FL_IOCTL_MOUNT_VOLUME/FL_MOUNT failed %d\n", out.status);
        }
    }

    /* WARNING: You must kill DiskOnChip driver and reboot your system now ! */
    return rc;
}
#define MAC_BYTES	6
#define MAC_CHARS	(MAC_BYTES*2)
#define MAC_TCHARS_BYTELEN	( MAC_CHARS*sizeof(u_int16_t) )
//address - could be NULL
int fl_ioctl_get_mac(int fd,unsigned char* address,char *buffer)
{
	XatIoctlInputRecord inrec;
	XatIoctlOutputRecord outrec;
	flIOctlRecord      ioctl_data;
	int                rc = 0;
	u_int16_t* utf16_buf[MAC_CHARS];
	
	inrec.InputBuffer		= address	;
	inrec.InputBufferSize	= address?sizeof(unsigned char*):0	;
	inrec.OutputBufferSize	= sizeof(utf16_buf)	;
	inrec.OutputBuffer		= utf16_buf		;

	ioctl_data.inputRecord = &inrec;
	ioctl_data.outputRecord = &outrec;
	
	rc = ioctl (fd, FL_IOCTL_HAL_GET_MAC, &ioctl_data);
	if (rc != 0)// || (outrec.BytesReturned != inrec.OutputBufferSize))
    {
//    	if(0 == rc)
//    	{
//    		rc = -1 ;
//    	}
        fprintf (stderr, "FL_IOCTL_HAL_GET_MAC failed.\n");
    }
	else
	{
		gchar * temp = g_utf16_to_utf8((gunichar2*)utf16_buf,MAC_CHARS,NULL,NULL,NULL);
		if(!temp)
		{
			fprintf (stderr, "FL_IOCTL_HAL_GET_MAC failed. No memory.\n");
			return -1;
		}
		memcpy(buffer,temp,MAC_CHARS);
	}
//	*pdwBytesReturned	= outrec.BytesReturned;
	return rc;	
}

int fl_ioctl_write_mac(int fd,unsigned char* address,const char *buffer)
{
	XatIoctlInputRecord inrec;
	XatIoctlOutputRecord outrec;
	flIOctlRecord      ioctl_data;
	int                rc = 0;
	char mac_buffer[sizeof(unsigned int) + MAC_TCHARS_BYTELEN];
	
	*(unsigned int*)mac_buffer = (unsigned int)address;
	
	gunichar2 *temp = g_utf8_to_utf16(buffer,MAC_CHARS,NULL,NULL,NULL);
	if(! temp)
	{
		return -1;
	}
	memcpy(&mac_buffer[4],temp, MAC_TCHARS_BYTELEN);
	g_free(temp);temp = NULL;
	
	inrec.InputBuffer		= mac_buffer;
	inrec.InputBufferSize	= sizeof(mac_buffer);
	inrec.OutputBufferSize	= 0;
	inrec.OutputBuffer		= NULL;
	
	ioctl_data.inputRecord = &inrec;
	ioctl_data.outputRecord = &outrec;
		
	rc = ioctl (fd, FL_IOCTL_HAL_SET_MAC, &ioctl_data);
	if (rc != 0) 
    {    
        fprintf (stderr, "FL_IOCTL_HAL_SET_MAC failed.\n");
    }	
	return rc;
}

