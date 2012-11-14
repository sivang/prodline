#define _IN_APP_
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "tffsioct.h"
#include "Xat/xatioctl.h"
#include "limits.h"
#include <unistd.h>
#include <errno.h>
#include <err.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#define MAX_PARTITIONS	14
typedef struct _tagFORMATARGUMENTS
{
	gboolean bForceYes;
	gint number_of_partitions;
	BDTLPartitionFormatParams3   partitions[MAX_PARTITIONS];
	gchar* ipl_file;
	gchar* mac_string;
	gboolean read_mac;
	gboolean show_unique;
}FORMATARGUMENTS;

FORMATARGUMENTS g_FormatArguments;

gboolean check_global_struct()
{
	gboolean res = TRUE;
	unsigned i = 0;
	if( (g_FormatArguments.number_of_partitions <= 0) || (g_FormatArguments.number_of_partitions > MAX_PARTITIONS ) )
	{
		fprintf(stderr,"Please specify number of partitions between 1 and %d\n",MAX_PARTITIONS);
		res = FALSE; goto End;
	}
	for(i = 0; i < g_FormatArguments.number_of_partitions; i++)
	{
		if( ! g_FormatArguments.partitions[i].length )
		{
			fprintf(stderr,"Length for partition %d of %d was not specified.\n",i+1,g_FormatArguments.number_of_partitions);
			res = FALSE; goto End;
		}
	}
	
End:
	return res ;
}

char* LenTypeToString(FLWord lengthType)
{
	switch(lengthType)
	{
		case FL_LENGTH_IN_BYTES:
			return "bytes";
		case FL_LENGTH_IN_SECTORS:
			return "sectors";
		case FL_LENGTH_IN_PERCENTS:
			return "%";
		default :
			return "???";
	};
}

char* ProtectionToString(FLByte protectionType,FLByte protectionKey[8])
{
	static char temp[256];
	if(!protectionType)
	{
		return "No";
	}
	
	temp[0] = 0 ;
	
	if(protectionType & PROTECTABLE)
	{
		strcat(temp, "PROTECTABLE ");		
	}
	
	if(protectionType & READ_PROTECTED)
	{
		strcat(temp, "READ_PROTECTED ");		
	}
	
	if(protectionType & WRITE_PROTECTED)
	{
		strcat(temp, "WRITE_PROTECTED ");		
	}
	
	if(protectionType & CHANGEABLE_PROTECTION)
	{
		strcat(temp, "CHANGEABLE_PROTECTION ");		
	}
	return temp;
}

void print_global_struct()
{
	unsigned i;
	printf("======================== Going to format the disk. Please check the settings to be used :\n");
	printf("Number of partitions: %d\n",g_FormatArguments.number_of_partitions);
	for( i = 0; i < g_FormatArguments.number_of_partitions; i++)
	{
		printf("--- Partition %u ---\n"
				"Length: %lu %s\n"
				"Fast area: %lu %s\n"
				"Protection: %s\n"
				,i+1,
				g_FormatArguments.partitions[i].length,
				LenTypeToString(g_FormatArguments.partitions[i].lengthType),
				g_FormatArguments.partitions[i].fastAreaLength,
				LenTypeToString(g_FormatArguments.partitions[i].fastAreaLengthType),
				ProtectionToString(g_FormatArguments.partitions[i].protectionType,
											g_FormatArguments.partitions[i].protectionKey)
				);		
	}
}

gboolean ParseMACAddress (const gchar *option_name,const gchar *value,gpointer data,GError **error)
{
#define TFFS_MAC_STRLEN	12
	size_t len = 0;
	
	char mac[TFFS_MAC_STRLEN+1];
	char* pmac = mac;	
	
	//check length
	len = strlen(value);
	if(len < TFFS_MAC_STRLEN)
	{
		g_set_error(error,0,0,"MAC address is too short in option '%s'",option_name);
		return FALSE;
	}	
	if(len > TFFS_MAC_STRLEN + 5 )// form XX:XX...
	{
		g_set_error(error,0,0,"MAC address is too long in option '%s'",option_name);
		return FALSE;
	}
	while(*value)
	{
		if(':' != *value )
		{
			if( ! isxdigit(*value) )
			{
				g_set_error(error,0,0,"MAC address should be in form XX:XX:XX:XX:XX:XX or XXXXXXXXXXXX in option '%s'",option_name);
				return FALSE;
			}
			*pmac = (char)toupper(*value);
			pmac ++ ;
		}
		value ++;
	}//for
	*pmac = 0;
	if(strlen(mac) != TFFS_MAC_STRLEN)
	{
		g_set_error(error,0,0,"MAC address should be formed as XX:XX:XX:XX:XX:XX or XXXXXXXXXXXX in option '%s'",option_name);
		return FALSE;
	}
	g_FormatArguments.mac_string = g_strdup(mac);
	return TRUE;
}

gboolean ParsePartitionArgument (const gchar *option_name,const gchar *value,gpointer data,GError **error)
{
	int partition_index = 0;
	gboolean bres = TRUE;
	*error = NULL;
	gchar** text_params = NULL;
	FLByte len_type = 0;
	FLByte fast_len_type = 0;
	unsigned long len = 0;
	unsigned long fast_area_len = 0;
	char *endptr = NULL;
	FLByte protectionType = 0xFF;
	gchar*  temp_string;
	
	if( (option_name[1] >= '0') && (option_name[1] <= '9') )
	{
		partition_index = option_name[1] - 0x30;
	}
	else if( (option_name[1] >= 'A') && (option_name[1] <= 'D') )
	{
		partition_index = 10 + option_name[1] - 'A' ;
	}
	else
	{		
		g_set_error(error,0,0,"Unknown argument '%s'",option_name);
		return FALSE;
	}	
	
	temp_string = g_ascii_strdown (value,-1);
	
	text_params = g_strsplit(temp_string, ",",5);//4 parameters and remainder if any (errorneous)
	g_free(temp_string);temp_string = NULL;
	if(0 == text_params[0][0])
	{
		g_set_error(error,0,0,"Partition length in argument '%s' not specified.",option_name);
		bres = FALSE;
		goto End;
	}
	if((NULL == text_params[1]) || (0 == text_params[1][0]) )
	{
		g_set_error(error,0,0,"Fast area length in argument '%s' not specified.",option_name);
		bres = FALSE;
		goto End;
	}
	if(text_params[2] && text_params[2][0] && (!text_params[3] || !text_params[3][0]) )	
	{
		g_set_error(error,0,0,"Protection flag specified w/o protection type in argument '%s'.",option_name);
		bres = FALSE;
		goto End;
	}
/////////////////////////// partition length	
	endptr = NULL;
	len = strtoul(text_params[0], &endptr, 10);
	if( (0 == len) && endptr && (endptr[0] == text_params[0][0] ) )
	{
		g_set_error(error,0,0,"Partition length should be specified as number and not '%s' in argument '%s'.",
						text_params[0], option_name);
		bres = FALSE;
		goto End;		
	}
	
	if(!endptr || !(*endptr))
	{
		len_type = FL_LENGTH_IN_BYTES;
		len *= 1024;
	}
	else switch (endptr[0])
	{			
		case 'b':
			len_type = FL_LENGTH_IN_BYTES;
			break;
		case 'k':
			len_type = FL_LENGTH_IN_BYTES;
			len *= 1024;
			break;
		case 'm':
			len_type = FL_LENGTH_IN_BYTES;
			len *= (1024*1024);
			break;
		case 's':
			len_type = FL_LENGTH_IN_SECTORS;
			break;
		default:
			g_set_error(error,0,0,"Length type '%s' is misunderstood in argument '%s'.",
								endptr, option_name);
			bres = FALSE;
			goto End;
	};	
/////////////////////////// fast area length
	endptr = NULL;
	fast_area_len = strtoul(text_params[1], &endptr, 10);
	if( (0 == len) && endptr && (endptr[0] == text_params[1][0] ) )
	{
		g_set_error(error,0,0,"Fast area length should be specified as number and not '%s' in argument '%s'.",
						text_params[1], option_name);
		bres = FALSE;
		goto End;		
	}
	
	if(!endptr || !(*endptr))
	{
		fast_len_type = FL_LENGTH_IN_PERCENTS;
	}
	else switch (endptr[0])
	{
		case '%':
			fast_len_type = FL_LENGTH_IN_PERCENTS;
			break;
		case 'k':
			fast_len_type = FL_LENGTH_IN_BYTES;
			fast_area_len *= 1024;
			break;
		case 'm':
			fast_len_type = FL_LENGTH_IN_BYTES;
			fast_area_len *= (1024*1024);
			break;
		case 'b':
			fast_len_type = FL_LENGTH_IN_BYTES;
			break;		
		case 's':
			fast_len_type = FL_LENGTH_IN_SECTORS;
			break;
		default:
			g_set_error(error,0,0,"Fast area length type '%s' is misunderstood in argument '%s'.",
								endptr, option_name);
			bres = FALSE;
			goto End;
	};
	/////////////////////////// protection type
	if(! text_params[2] || ! text_params[2][0] )
	{
		bres = TRUE;
		goto End;
	}
	protectionType = 0;
	if(strchr(text_params[2],'P'))
	{
		protectionType |= PROTECTABLE;			
	}
	if(strchr(text_params[2],'W'))
	{
		protectionType |= WRITE_PROTECTED;			
	}
	if(strchr(text_params[2],'R'))
	{
		protectionType |= (READ_PROTECTED|WRITE_PROTECTED);			
	}
	if(strchr(text_params[2],'C'))
	{
		protectionType |= CHANGEABLE_PROTECTION;			
	}
	if( strlen(text_params[3]) > sizeof(g_FormatArguments.partitions[0].protectionKey) )
	{
		g_set_error(error,0,0,"Protection key '%s' is too long in argument '%s'.",
				text_params[2], option_name);
		bres = FALSE;
		goto End;		
	}
End:
	if(bres)
	{
		BDTLPartitionFormatParams3 *fp3 = &(g_FormatArguments.partitions[partition_index]);
		fp3->length = len;
		fp3->fastAreaLength = fast_area_len;
		fp3->lengthType = len_type;
		fp3->fastAreaLengthType = fast_len_type;
		if(protectionType != 0xFF)
		{
			fp3->protectionType = protectionType;
			bzero(fp3->protectionKey,sizeof(fp3->protectionKey));			
			memcpy(fp3->protectionKey,text_params[3],strlen(text_params[3]));
		}
	}
	if(text_params)
	{
		g_strfreev(text_params) ;
	}
	return bres;
}

gboolean parse_arguments(int argc, char* argv[])
{	
	GError *error = NULL;
	gboolean bres = TRUE;
	GOptionEntry options[] = {
			{"yes",'y',0,G_OPTION_ARG_NONE,&g_FormatArguments.bForceYes,
								"\"don't ask any questions, just do the job\"",NULL},
			{"number",'n',0,G_OPTION_ARG_INT,&g_FormatArguments.number_of_partitions,
											"specifies number of partitions form 1 to 14",NULL},
			{"0",'0',0,G_OPTION_ARG_CALLBACK,ParsePartitionArgument,
				"-0,1,2...D - HEXADECIMAL number from 0 to D specifies parameters for \n"
							"\t\t\tformatting 1st,2nd ...Nth partition accordingly.\n"
							"\t\t\t<len> - length of partition in bytes(B), kilobytes(K)(default), megabytes(M)\n"
								"\t\t\t\tor sectors(S) like 1000B,15K,10M, 150S, etc ...\n"
							"\t\t\t<fast area len> - length of additional, caching space for partition in\n"
								"\t\t\t\tpercents(default or '%'),\n"
								"\t\t\t\tbytes(B),kilobytes, megabytes or sectors, like 100%,3000B, etc ...\n"
							"\t\t\t<protection type> - 'W' for WRITE PROTECTED or 'R' for READ/WRITE PROTECTED combine.\n"
								"\n\t\t\tThose can be cobined with 'C' for CHANGEABLE PROTECTION.\n",NULL},
			{"1",'1',G_OPTION_FLAG_HIDDEN,G_OPTION_ARG_CALLBACK,ParsePartitionArgument,NULL,NULL},
			{"2",'2',G_OPTION_FLAG_HIDDEN,G_OPTION_ARG_CALLBACK,ParsePartitionArgument,NULL,NULL},
			{"3",'3',G_OPTION_FLAG_HIDDEN,G_OPTION_ARG_CALLBACK,ParsePartitionArgument,NULL,NULL},
			{"4",'4',G_OPTION_FLAG_HIDDEN,G_OPTION_ARG_CALLBACK,ParsePartitionArgument,NULL,NULL},
			{"5",'5',G_OPTION_FLAG_HIDDEN,G_OPTION_ARG_CALLBACK,ParsePartitionArgument,NULL,NULL},
			{"6",'6',G_OPTION_FLAG_HIDDEN,G_OPTION_ARG_CALLBACK,ParsePartitionArgument,NULL,NULL},
			{"7",'7',G_OPTION_FLAG_HIDDEN,G_OPTION_ARG_CALLBACK,ParsePartitionArgument,NULL,NULL},
			{"8",'8',G_OPTION_FLAG_HIDDEN,G_OPTION_ARG_CALLBACK,ParsePartitionArgument,NULL,NULL},
			{"9",'9',G_OPTION_FLAG_HIDDEN,G_OPTION_ARG_CALLBACK,ParsePartitionArgument,NULL,NULL},
			{"A",'A',G_OPTION_FLAG_HIDDEN,G_OPTION_ARG_CALLBACK,ParsePartitionArgument,NULL,NULL},
			{"B",'B',G_OPTION_FLAG_HIDDEN,G_OPTION_ARG_CALLBACK,ParsePartitionArgument,NULL,NULL},
			{"C",'C',G_OPTION_FLAG_HIDDEN,G_OPTION_ARG_CALLBACK,ParsePartitionArgument,NULL,NULL},
			{"D",'D',G_OPTION_FLAG_HIDDEN,G_OPTION_ARG_CALLBACK,ParsePartitionArgument,NULL,NULL},			
			{"ipl",'i',0,G_OPTION_ARG_FILENAME,&(g_FormatArguments.ipl_file),
					"write IPL from file. When specified along with formatting options, will be performed AFTER format is done.\n",NULL},
			{"read-mac",'r',0,G_OPTION_ARG_NONE,&g_FormatArguments.read_mac,"show current MAC address",NULL},
			{"write-mac",'w',0,G_OPTION_ARG_CALLBACK,ParseMACAddress,"write new MAC address in form XX:XX:XX:XX:XX:XX or XXXXXXXXXXXX\n",NULL},
			{"unique",'u',0,G_OPTION_ARG_NONE,&g_FormatArguments.show_unique,"show DOC Unique ID ",NULL},
			{NULL}
	};

	GOptionContext *ctx;

    ctx = g_option_context_new("[-y] -n <number> -1 <len>,<fast area len>[,<protection type>,<protection key 8 bytes>] -2... -N:....");
    g_option_context_add_main_entries(ctx, options, "tffs_format");
    bres = g_option_context_parse(ctx, &argc, &argv, &error);
    if(!bres)
    {
    	fprintf (stderr, "Bad argument: %s\n", error->message);    	
    }
    g_option_context_free(ctx);
    return bres;
}

int dismount_dev(char* dev_name);

int dismount_all_volumes()
{
	struct stat st_buf;
	char path[] = "/dev/tffsX" ;
	size_t last_index = sizeof(path) -2;
	int res = 0;
	char c = 0;
		
	for(c = 'b'; c < ('b'+MAX_PARTITIONS); c++)
	{
		path[last_index] = c;
		if(0 == (res = stat(path,&st_buf) ) )
		{					
			res = dismount_dev(path);			
		}
	}//for
	
	return 0;
}
int fl_ioctl_flash_format (int fd, int nPartitions,BDTLPartitionFormatParams3  partitions[]);
int dismount_dev_fd(int fd,FLStatus* pstat);
int fl_ioctl_ipl(int fd, char* buf, size_t buf_size);

int format_tffsa()
{
	int res = 0;
	FLStatus status  = flOK;
	int fd = open("/dev/tffsa",O_RDWR);
	if(-1 == fd)
	{
		res = errno;
		fprintf(stderr, "Cannot open device 'dev/tffsa'. %s", strerror(res));		
		goto End;
	}
	dismount_dev_fd(fd,&status);
	res = fl_ioctl_flash_format ( fd, g_FormatArguments.number_of_partitions,g_FormatArguments.partitions);
End:
	if(-1 != fd)
	{
		close(fd);
	}
	return res ;
}

int ipl_tffsa()
{
	int res = 0;
//	FLStatus status  = flOK;
	int fd = open("/dev/tffsa",O_RDWR);
	if(-1 == fd)
	{
		res = errno;
		fprintf(stderr, "Cannot open device 'dev/tffsa'. %s", strerror(res));		
		goto End;
	}
	gchar *contents = NULL;
	gsize length = 0;
	GError *error = NULL;
	gboolean bres = g_file_get_contents (g_FormatArguments.ipl_file,&contents,&length,&error);
	if(!bres)
	{
		fprintf(stderr,"Canot read file '%s'.%s",g_FormatArguments.ipl_file,error->message);
		g_error_free(error);error = NULL;
		goto End;
	}
	
	res = fl_ioctl_ipl ( fd,contents, length);
End:
	if(-1 != fd)
	{
		close(fd);
	}
	return res ;
}

void TryFormat()
{
	char answer = 'y';
	if(check_global_struct())
	{
		if( !g_FormatArguments.bForceYes)
		{			
			print_global_struct();
			printf("\nProceed with format ? (Y/n):");
			scanf("%c",&answer);			
		}
		if('y' == tolower((int)answer))
		{			
			//dismount all volumes
			printf("Dismounting all volumes in /dev/tffs*... ");
			if(0!= dismount_all_volumes())
			{
				printf("Failed !\n");
				exit(1);
			}
			else printf("Ok !\n");
			//call format
			printf("Formatting ... \n");
			printf( (0 == format_tffsa()) ? "Success !\n": "Failuire.\n");
			
		}//if(yes)
	}
}

void TryIpl()
{
	char answer = 'y';
	if( !g_FormatArguments.bForceYes)
	{			
		printf("You are going to write file '%s' onto IPL area of the DOC.\n"
				"Would you like to proceed? (Y/n):",g_FormatArguments.ipl_file);
		scanf("%c",&answer);			
	}
	if('y' == tolower((int)answer))
	{	
		printf("Writing IPL ...");
		printf( (0 == ipl_tffsa()) ? "Success !\n": "Failuire.\n");	
	}
}

int fl_ioctl_read_unique(int fd,unsigned char *buffer16);
void PrintUnique()
{
	int res = 0;	
	unsigned char buffer[16];
	bzero(buffer,sizeof(buffer));
	int fd = open("/dev/tffsa",O_RDWR);
	if(-1 == fd)
	{
		res = errno;
		fprintf(stderr, "Cannot open device 'dev/tffsa'. %s", strerror(res));		
		goto End;
	}
	res = fl_ioctl_read_unique(fd,buffer);
	if(0 == res)
	{
		printf("DOC Unique ID is : %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n",
				buffer[0],buffer[1],buffer[2],buffer[3],
				buffer[4],buffer[5],buffer[6],buffer[7],
				buffer[8],buffer[9],buffer[10],buffer[11],
				buffer[12],buffer[13],buffer[14],buffer[15]);
	}
End:
	if(-1 != fd)
	{
		close (fd);	
	}
}

int fl_ioctl_get_mac(int fd,unsigned char* address,unsigned char *buffer);
void PrintMac()
{
	int res = 0;	
	unsigned char buffer[12+1];
	bzero(buffer,sizeof(buffer));
	int fd = open("/dev/tffsa",O_RDWR);
	if(-1 == fd)
	{
		res = errno;
		fprintf(stderr, "Cannot open device 'dev/tffsa'. %s", strerror(res));		
		goto End;
	}
	res = fl_ioctl_get_mac(fd,NULL,buffer);
	buffer[12] = 0;
	printf("MAC address is: '%s'\n",buffer);	
	
End:
	if(-1 != fd)
	{
		close (fd);	
	}
}
int fl_ioctl_write_mac(int fd,unsigned char* address,const char *buffer);
void WriteMac(const char* mac)
{
	int res = 0;	
	int fd = open("/dev/tffsa",O_RDWR);
	if(-1 == fd)
	{
		res = errno;
		fprintf(stderr, "Cannot open device 'dev/tffsa'. %s", strerror(res));		
		goto End;
	}	
	res = fl_ioctl_write_mac(fd,NULL,mac);
	if(0 == res)
	{
		printf( "MAC written successfully.\n");
	}
End:
	if(-1 != fd)
	{
		close (fd);	
	}
}

int main(int argc, char* argv[])
{	
	unsigned u = 0;	
	bzero(&g_FormatArguments,sizeof(g_FormatArguments));
	for(u = 0; u< MAX_PARTITIONS; u ++)
	{
		g_FormatArguments.partitions[u].fastAreaVirtualFactor = 1;
	}
		
	if(!parse_arguments(argc,argv)) return 1;
	if(g_FormatArguments.number_of_partitions)
	{
		TryFormat();
	}
	if(g_FormatArguments.ipl_file && g_FormatArguments.ipl_file[0])
	{
		TryIpl();
	}	
	if(g_FormatArguments.mac_string)
	{	
		WriteMac(g_FormatArguments.mac_string);
	}
	if(g_FormatArguments.show_unique)
	{
		PrintUnique();		
	}
	if(g_FormatArguments.read_mac)
	{
		PrintMac();
	}
	
	return 0 ;
}

