/********************************************************************************/
/*										*/
/*	Copyright (C) 2014-2015, SUSE LLC					*/
/*										*/
/*	All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#define _GNU_SOURCE
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

/*
 *	Structures for internal usage.
 */

struct sysinfo_t {
	char	manufacturer[4];
	char	type[5];
	char	sequence[17];
	char	plant[3];
	char	model[24];
	int	cpus_total;
	int	cpus_configured;
	int	cpus_reserved;
	int	cpus_standby;
	int	lpar_number;
	char	lpar_name[9];
	char	lpar_characteristic[10];
	int	lpar_cpus_total;
	int	lpar_cpus_configured;
	int	lpar_cpus_shared;
	int	lpar_cpus_dedicated;
	char	vm00_name[9];
	char	vm00_control_programm[32];
	int	vm00_cpus_total;
	int	vm00_cpus_configured;
} sysinfo_t;
/*
 *	Data types
 */
enum datatypes {
	integer,
	string,
	floatingpoint
};

struct sysprog_attribute {
	char	*name;
	enum datatypes type;
	int	offset;
	int	size;
};

#define	SIZEOF_INPUTBUFFER_LINE	1024

#define	WITHOUT_KEY	0
#define	WITH_KEY	1
char *versionstring = "Version 0.8 2015-03-04 13:31";
struct sysinfo_t sys_info;

#define	SYS_INFO_OFFSET(a) (&(sys_info.a) - &sys_info)

/*
 *	delete whitespace at the beginning and end of a string
 */
#define trim(beginning, end) while (isspace(*beginning)) beginning++; \
	while (isspace(*end)) end--; \
	end++; *end = '\0'; end--;

/*
 *	This table specifies which strings from
 *	/proc/sysinfo are recognised.
 *	The table must be sorted by the string
 *	as searching in the table is done with bsearch
 */
struct sysprog_attribute attribute_table[] = {
	{"Adjustment 01-way", string, -1,0},
	{"Adjustment 02-way", string, -1,0},
	{"Adjustment 03-way", string, -1,0},
	{"Adjustment 04-way", string, -1,0},
	{"Adjustment 05-way", string, -1,0},
	{"Adjustment 06-way", string, -1,0},
	{"Adjustment 07-way", string, -1,0},
	{"Adjustment 08-way", string, -1,0},
	{"Adjustment 09-way", string, -1,0},
	{"Adjustment 10-way", string, -1,0},
	{"Adjustment 11-way", string, -1,0},
	{"Adjustment 12-way", string, -1,0},
	{"Adjustment 13-way", string, -1,0},
	{"Adjustment 14-way", string, -1,0},
	{"Adjustment 15-way", string, -1,0},
	{"Adjustment 16-way", string, -1,0},
	{"Adjustment 17-way", string, -1,0},
	{"Adjustment 18-way", string, -1,0},
	{"Adjustment 19-way", string, -1,0},
	{"Adjustment 20-way", string, -1,0},
	{"Adjustment 21-way", string, -1,0},
	{"Adjustment 22-way", string, -1,0},
	{"Adjustment 23-way", string, -1,0},
	{"Adjustment 24-way", string, -1,0},
	{"Adjustment 25-way", string, -1,0},
	{"Adjustment 26-way", string, -1,0},
	{"Adjustment 27-way", string, -1,0},
	{"Adjustment 28-way", string, -1,0},
	{"Adjustment 29-way", string, -1,0},
	{"Adjustment 30-way", string, -1,0},
	{"CPU Topology HW", string, -1,0},
	{"CPU Topology SW", string, -1,0},
	{"CPUs Configured", integer, offsetof(struct sysinfo_t,cpus_configured), sizeof(sys_info.cpus_configured)},
	{"CPUs Reserved", integer, offsetof(struct sysinfo_t,cpus_reserved), sizeof(sys_info.cpus_reserved)},
	{"CPUs Standby", integer, offsetof(struct sysinfo_t,cpus_standby), sizeof(sys_info.cpus_standby)},
	{"CPUs Total", integer, offsetof(struct sysinfo_t,cpus_total), sizeof(sys_info.cpus_total)},
	{"Capability", string, -1,0},
	{"Capacity Adj. Ind.", string, -1,0},
	{"Capacity Ch. Reason", string, -1,0},
	{"Capacity Transient", string, -1,0},
	{"LPAR Adjustment", string, -1,0},
	{"LPAR CPUs Configured", integer, offsetof(struct sysinfo_t,lpar_cpus_configured), sizeof(sys_info.lpar_cpus_configured)},
	{"LPAR CPUs Dedicated", integer, offsetof(struct sysinfo_t,lpar_cpus_dedicated), sizeof(sys_info.lpar_cpus_dedicated)},
	{"LPAR CPUs Reserved", string, -1,0},
	{"LPAR CPUs Shared", integer, offsetof(struct sysinfo_t,lpar_cpus_shared), sizeof(sys_info.lpar_cpus_shared)},
	{"LPAR CPUs Standby", string, -1,0},
	{"LPAR CPUs Total", integer, offsetof(struct sysinfo_t,lpar_cpus_total), sizeof(sys_info.lpar_cpus_total)},
	{"LPAR Characteristics", string, offsetof(struct sysinfo_t,lpar_characteristic), sizeof(sys_info.lpar_characteristic)},
	{"LPAR Name", string, offsetof(struct sysinfo_t,lpar_name), sizeof(sys_info.lpar_name)},
	{"LPAR Number", integer, offsetof(struct sysinfo_t,lpar_number), sizeof(sys_info.lpar_number)},
	{"Manufacturer", string, offsetof(struct sysinfo_t,manufacturer), sizeof(sys_info.manufacturer)},
	{"Model", string, offsetof(struct sysinfo_t, model),sizeof(sys_info.model)},
	{"Model Capacity", string, -1,0},
	{"Model Perm. Capacity", string, -1,0},
	{"Model Temp. Capacity", string, -1,0},
	{"Nominal Cap. Rating", string, -1,0},
	{"Nominal Capability", string, -1,0},
	{"Nominal Perm. Rating", string, -1,0},
	{"Nominal Temp. Rating", string, -1,0},
	{"Plant", string, -1,0},
	{"Secondary Capability", string, -1,0},
	{"Sequence Code", string, offsetof(struct sysinfo_t,sequence), sizeof(sys_info.sequence)},
	{"Type", string, offsetof(struct sysinfo_t,type), sizeof(sys_info.type)},
	{"Type 1 Percentage", string, -1,0},
	{"Type 2 Percentage", string, -1,0},
	{"Type 3 Percentage", string, -1,0},
	{"Type 4 Percentage", string, -1,0},
	{"Type 5 Percentage", string, -1,0},
	{"VM00 Adjustment", string, -1,0},
	{"VM00 CPUs Configured", integer, offsetof(struct sysinfo_t,vm00_cpus_configured), sizeof(sys_info.vm00_cpus_configured)},
	{"VM00 CPUs Reserved", string, -1,0},
	{"VM00 CPUs Standby", string, -1,0},
	{"VM00 CPUs Total", integer, offsetof(struct sysinfo_t,vm00_cpus_total), sizeof(sys_info.vm00_cpus_total)},
	{"VM00 Control Program", string, offsetof(struct sysinfo_t,vm00_control_programm), sizeof(sys_info.vm00_control_programm)},
	{"VM00 Name", string, offsetof(struct sysinfo_t,vm00_name), sizeof(sys_info.vm00_name)},
};

/*
 *	List of machine types
 */
struct machinetype {
	char	*typenumber;
	char	*fullname;
	} machinetypes[] =  {
	{ "2064", "2064 = z900    IBM eServer zSeries 900" },
	{ "2066", "2066 = z800    IBM eServer zSeries 800" },
	{ "2084", "2084 = z990    IBM eServer zSeries 990" },
	{ "2086", "2086 = z890    IBM eServer zSeries 890" },
	{ "2094", "2094 = z9-EC   IBM System z9 Enterprise Class" },
	{ "2096", "2096 = z9-BC   IBM System z9 Business Class" },
	{ "2097", "2097 = z10-EC  IBM System z10 Enterprise Class" },
	{ "2098", "2098 = z10-BC  IBM System z10 Business Class" },
	{ "2817", "2817 = z196    IBM zEnterprise 196" },
	{ "2818", "2818 = z114    IBM zEnterprise 114" },
	{ "2827", "2827 = z12-EC  IBM zEnterprise EC12" },
	{ "2828", "2828 = z12-BC  IBM zEnterprise BC12" },
	{ "2964", "2964 = z13     IBM z13" },
	};

int	debug = 0;

/******************************************************************************/
/*									      */
/*	Compare two attributes by name					      */
/*									      */
/******************************************************************************/
int	cmp_sysprog_attribute(const void *p1, const void *p2)
{
	struct sysprog_attribute *a1 = (struct sysprog_attribute *) p1;
	struct sysprog_attribute *a2 = (struct sysprog_attribute *) p2;
	return strcmp(a1->name, a2->name);
} /* cmp_sysprog_attribute */

/******************************************************************************/
/*									      */
/*	Store the value, if it is found in the different lists		      */
/*	String has the form						      */
/*	xxxxxxx:yyyyyyy							      */
/*	xxxxxxx and yyyyyy are all characters excluding ":"		      */
/*	remove leading and trailing space (according to isspace)	      */
/*	from xxxxxxx and yyyyyy						      */
/*									      */
/******************************************************************************/
void store_value(char *sv_buffer)
{
int	int_value,
	*int_value_ptr;
char	*buffer_t_anf = sv_buffer;
char	*delimiter, *key, *value;
char	*buffer_t_end = sv_buffer + strlen(sv_buffer) -1;
struct sysprog_attribute	*erg,searchvalue;


	trim(buffer_t_anf, buffer_t_end);
	if (buffer_t_anf >= buffer_t_end) return;
	delimiter = strchr(buffer_t_anf,':');
	if (delimiter == NULL) return;
	key = buffer_t_anf;
	value = delimiter + 1;
	*delimiter = '\0';
	delimiter--;
	trim(key,delimiter);
	trim(value,buffer_t_end);
	searchvalue.name = key;
	searchvalue.type = 0;
	searchvalue.offset = -1;
	erg = (struct sysprog_attribute *) bsearch(&searchvalue,
						   attribute_table,
						   sizeof(attribute_table) / sizeof(struct sysprog_attribute),
						   sizeof(struct sysprog_attribute),
						   cmp_sysprog_attribute);
	if (erg == NULL) {
		fprintf(stderr,"Error: (key \"%s\" not found)\n", key);
		return;
		/* Value not found */
	} /* endif */
	if ((debug & 4) == 4) {
		printf("Found %s Size %d Offset %d",erg->name, erg->size, erg->offset);
	} /* endif */
	if (erg->size == 0) {
		if ((debug & 4) == 4) printf("\n");
		return;
	}
	switch (erg->type)
	   {
	   case integer:
	   		int_value = atoi(value);
			int_value_ptr = (int *) ((char *)(&sys_info) + erg->offset);
			*int_value_ptr = int_value;
	   	break;
	   case string:
	   		strncpy((char *)(&sys_info) + erg->offset, value, erg->size);
	   	break;
	   case floatingpoint:
	   	break;
	   default:
	      break;
	   } /* endswitch */
	if ((debug & 4) == 4) {
		printf("  Value \"%s\"\n",value);
	} /* endif */
	return;
} /* store_value */

/******************************************************************************/
/*									      */
/*	Read /proc/sysinfo in and store it				      */
/*									      */
/******************************************************************************/
int read_sysinfo()
{
FILE	*sys_input = NULL;
char	*r_buffer = malloc(SIZEOF_INPUTBUFFER_LINE);
size_t	size_input = SIZEOF_INPUTBUFFER_LINE;
char	*path;
int	line,i;

	path = ((debug & 1) == 1 ? "sysinfo.zvm" : "/proc/sysinfo");
	sys_input = fopen(path, "r");
	if (sys_input == NULL)
	  {
	  fprintf(stdout,"Unable to open \"%s\" (%s)\n.",path, strerror(errno));
	  return 1;
	  } /* endif */
	line = 0;
	while ((i= getline(&r_buffer, &size_input, sys_input)) != -1)
	     {
	     if ((debug & 2) == 2) {
	     	printf("Line %d Number %d \"%s\"\n", line, i, r_buffer);
	     } /* endif */
	     store_value(r_buffer);
	     line++;
	     } /* endwhile */
	return 0;
} /* read_sysinfo  */

/******************************************************************************/
/*									      */
/*	List all attributes we can match or all attribute we really match     */
/*									      */
/******************************************************************************/
void list(char *type)
{
int	i;
int	attr, recog;
struct sysprog_attribute	*tmp_sysprog;
	
	attr = recog = 0;
	attr = !strcmp("Attribute", type);
	recog = !strcmp("Recognised", type);
	tmp_sysprog = attribute_table;
	for (i = 0; i < (sizeof(attribute_table) / sizeof(struct sysprog_attribute)); i++, tmp_sysprog++)
	   {
	   if (recog && (tmp_sysprog->offset != -1)) {
	   	printf("%s\n",tmp_sysprog->name);
	   } /* endif */
	   if (attr) {
	   	printf("%s\n",tmp_sysprog->name);
	   } /* endif */
	   } /* endfor */
return;
} /* list  */

/******************************************************************************/
/*									      */
/*	Look for one attribute and print it				      */
/*									      */
/******************************************************************************/
void print_attribute(char *user_string, char* attribute, int print_key)
{ 
char	*value;
int	int_value;
struct sysprog_attribute	*erg,searchvalue;

	searchvalue.name = attribute;
	searchvalue.type = 0;
	searchvalue.offset = -1;
	if (debug & 8) {
		printf("Gesucht %s\n", attribute);
	} /* endif */
	erg = (struct sysprog_attribute *) bsearch(&searchvalue,
						   attribute_table,
						   sizeof(attribute_table) / sizeof(struct sysprog_attribute),
						   sizeof(struct sysprog_attribute),
						   cmp_sysprog_attribute);
	if (debug & 8) {
		printf("Gefunden %s Offset %d Type %d\n", erg->name, erg->offset, erg->type);
	} /* endif */
	if (erg != NULL) {
		if (erg->offset != -1) {
			if (print_key == WITH_KEY) {
				printf("%s: ",(user_string == NULL? attribute: user_string));
			} /* endif */
			value = (char *) &sys_info + erg->offset;
			switch (erg->type)
			   {
			   case integer:
			   	int_value = *((int *) value);
				printf("%d\n", int_value);
			   	break;
			   case string:
				printf("%s\n", value);
			   	break;
			   case floatingpoint:
			   	break;
			   default:
			      break;
			   } /* endswitch */
		} /* endif */
	} /* endif */
} /* print_attribute  */

/********************************************************************************/
/*										*/
/*	Look at the type of machine we're running on and print out a user	*/
/*	friendly string								*/
/*										*/
/********************************************************************************/
void print_cputype()
{
int	i, search;
char	*value;
struct sysprog_attribute	*erg,searchvalue;

	searchvalue.name = "Type";
	searchvalue.type = 0;
	searchvalue.offset = -1;
	erg = (struct sysprog_attribute *) bsearch(&searchvalue,
						   attribute_table,
						   sizeof(attribute_table) / sizeof(struct sysprog_attribute),
						   sizeof(struct sysprog_attribute),
						   cmp_sysprog_attribute);
	if ( erg != NULL) {
		value = ((char *) &sys_info) + erg->offset;
		for (i = 0, search = 1; (i < sizeof(machinetypes) / sizeof(struct machinetype)) && search ; i++)
		   {
		   if (strcmp(value, machinetypes[i].typenumber) == 0) {
		   	printf("%s\n", machinetypes[i].fullname);
			search = 0;
		   } /* endif */
		   } /* endfor */
		if (search != 0) {
			printf("An unknown machine type was reported: %s\n\
Please file a bug report with this output:\n" , value);
/*	TODO output of /proc/sysinfo					      */
		} /* endif */
	} /* endif */
	return;
} /* print_cputype  */

/********************************************************************************/
/*										*/
/*	Print out the values for SCC						*/
/*										*/
/*	To uniquely identify a machine the following information is used:	*/
/*										*/
/*	Type									*/
/*	Sequence code								*/
/*	CPUs total								*/
/*	CPUs IFL								*/
/*	LPAR Number								*/
/*	LPAR Characteristics:							*/
/*	LPAR CPUs								*/
/*	LPAR IFLs								*/
/*										*/
/*	Optional:								*/
/*										*/
/*	VM00 Name								*/
/*	VM00 Control Programm							*/
/*	VM00 CPUs								*/
/*										*/
/********************************************************************************/
void print_scc()
{
print_attribute(NULL,"Type", WITH_KEY);
print_attribute(NULL,"Sequence Code", WITH_KEY);
print_attribute(NULL,"CPUs Total", WITH_KEY);
print_attribute("CPUs IFL", "CPUs Configured", WITH_KEY);
print_attribute(NULL,"LPAR Number", WITH_KEY);
print_attribute(NULL,"LPAR Name", WITH_KEY);
print_attribute(NULL,"LPAR Characteristics", WITH_KEY);
print_attribute(NULL,"LPAR CPUs Total", WITH_KEY);
print_attribute("LPAR CPUs IFL", "LPAR CPUs Configured", WITH_KEY);
if (sys_info.vm00_cpus_total > 0) {
	print_attribute(NULL, "VM00 Name", WITH_KEY);
	print_attribute(NULL, "VM00 Control Program", WITH_KEY);
	print_attribute(NULL, "VM00 CPUs Total", WITH_KEY);
	print_attribute("VM00 IFLs", "VM00 CPUs Configured", WITH_KEY);
} /* endif */
return;
} /* print_scc */

/******************************************************************************/
/*									      */
/*	print out the uuid for this machine				      */
/*									      */
/*	TODO!								      */
/*									      */
/******************************************************************************/
void print_uuid()
{
return;
} /* print_uuid */


/******************************************************************************/
/*									      */
/*	Help Function							      */
/*									      */
/******************************************************************************/
void help()
{
puts("help:\n\
\n\
-a <attribute>	List the value of the named attribute\n\
-c		Print the cputype of this machine\n\
-d <number>	Debug Level\n\
-h		this help\n\
-L <keyword>	List the requested list (Attribute, Recognised)\n\
-s 		create Info for SCC\n\
-u		create uuid\n\
-V		print version string\n\
");
if (debug != 0) {
	puts("\n\
Valid values for debug:\n\
	1 - read sysinfo.zvm from current directory instead of /proc/sysinfo\n\
	2 - printout lines read in from source (see debug == 1)\n\
	4 - printf found keys in store_value\n\
	8 - Search expression in show attribute\n\
");
} /* endif */
} /* help  */

/******************************************************************************/
/*									      */
/*	Main								      */
/*									      */
/******************************************************************************/
int main(int argc, char **argv, char **envp)
{
int	opt;
int	read_sysinfo_opt;
int	print_attr;
int	print_cpu;
int	print_help;
int	list_attr;
int	create_scc;
int	create_uuid;
char 	*print_attribute_param = NULL;
char	*list_attribute_param = NULL;

	read_sysinfo_opt =
	print_attr	=
	print_cpu	=
	print_help	=
	list_attr	=
	create_scc	=
	create_uuid	= 0;
	memset(&sys_info,'\0', sizeof(sys_info));
	if (strcmp(argv[0],"cputype") == 0) {
		read_sysinfo_opt++;
		print_cpu++;
	} /* endif */
	else {
		while ((opt = getopt(argc, argv, "a:cd:hL:suV")) != -1) {
		   switch (opt)
		      {
		      case 'a':
				read_sysinfo_opt++;
				print_attr++;
				print_attribute_param = strdup(optarg);
			break;
		      case 'c':
				read_sysinfo_opt++;
				print_cpu++;
			break;
		      case 'd':
				debug = atoi(optarg);
			break;
		      case 'L':
				read_sysinfo_opt++;
				list_attr++;
				list_attribute_param = strdup(optarg);
			break;
		      case 's': /* create unique string for scc  */
				read_sysinfo_opt++;
				create_scc++;
			break;
		      case 'u': /* create UUID  */
				read_sysinfo_opt++;
				create_uuid++;
			break;
		      case 'V':
				printf("%s\n",versionstring);
				return 0;
			break;
		      case 'h':
		      default:
				print_help++;
			 break;
		      } /* endswitch */
		   } /* while */
	     } /* endlse */
	   if (print_help != 0) {
		help();
		return 0;
	   } /* endif */
	   if (read_sysinfo_opt != 0) {
	   	if (read_sysinfo() != 0) {
			return 99;
		} /* endif */
	   } /* endif */
	   if ((print_attr + print_cpu + list_attr + create_scc + create_uuid) > 1) {
	   	fputs("Only one of the options a, c, L, s or u can be specified.",stderr);
		return 1;
	   } /* endif */
	   if (print_attr != 0) {
	   	print_attribute(NULL, print_attribute_param, WITHOUT_KEY);
		return 0;
	   } /* endif */
	   if (print_cpu != 0) {
		print_cputype();
		return 0;
	   } /* endif */
	   if (list_attr != 0) {
		list(list_attribute_param);
		return 0;
	   } /* endif */
	   if (create_scc != 0) {
	   	print_scc();
		return 0;
	   } /* endif */
	   if (create_uuid != 0) {
	   	print_uuid();
		return 0;
	   } /* endif */
	   help();
return 0;
} /* end main */
