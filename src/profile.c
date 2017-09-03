/***************************************
 * This program is intended to provide
 * some functions to read & write .INI
 * format file in OS other than Windows
 * FORMAT:
 * [section]
 * entry=value
 * any line begin with '#' is ignored
 *
 * Date: Jan. 26, 2000
 * Modified: Jan. 10, 2005  [ÐÂÔöº¯Êý£¬È¡×Ö·û´®Êý×éºÍÕûÐÍÊý×é]
 *           Mar. 1, 2005   [ÐÞ¸Ä¡°ÔÚºÍ¹¤×÷Â·¾¶²»Í¬µÄÄ¿Â¼ÏÂÐ´¶à´ÎÊ§°Ü¡±µÄ´íÎó]
 ***************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* #define TEMP "temptemp.tmp" */
#define BUF_LEN  2048
#define MAX_ITEM 40

static char* BLANKS = " \t\r\n"; /* string containing all the candidate blank chars */

/*********************************************
 IsBlank:
  test if a character is a blank char
 Parameter:
  ch: the given character
 Return:
  TRUE or FALSE
 *********************************************/
static int IsBlank(char ch){
	char *p = BLANKS;
	
	while (*p) {
		if (ch == *p) return 1;
		p ++;
	}
	
	return 0;
}

/*********************************************
 IsBlankLine:
  test if a string containing all blank chars
 Parameter:
  line: a string
 Return:
  TRUE or FALSE  
 *********************************************/
static int IsBlankLine(char* line)
{
	if (line == NULL)
		return 0;
		
	while (*line != '\0') {
		if (!IsBlank(*line)) return 0;
		line++;
	}
	return 1;
}

/*********************************************
 StartsWith:
  test if a string starts with a substring
 Parameters:
  str:    the string
  substr: the sub string
  bIgnoreCase: case-sensetive or not
 Return:
  TRUE or FALSE
 *********************************************/
static int StartsWith(char* str, char* substr, int bIgnoreCase)
{
	if (str == NULL || substr == NULL)
		return 0;
	
	while (*substr!='\0' && *str!='\0') {
		char ch1 = *substr;
		char ch2 = *str;
		
		if (bIgnoreCase) {
			if (ch1>='A' && ch1<='Z') {
				ch1 = ch1-'A'+'a';
			}
			if (ch2>='A' && ch2<='Z') {
				ch2 = ch2-'A'+'a';
			}
		}
		
		if (ch1 != ch2) {
			return 0;
		}
		
		substr++;
		str++;
	}
	
	if (*substr == '\0')
		return 1;
	
	return 0;
}

/******************************************************
 WriteProfileString:
  write a string entry to the profile
 Parameters:
  profile: the name of the profile. If the file doesn't
		   exists, a new file will be created.
  section: the name of a section. If no such section
		   exists, the section will be inserted into
		   the profile.
  entry:   the name of an entry to be added or updated.
  value:   the value of the entry.
 Return:
  1: successful
  0: any error occurrs
 ******************************************************/
int WriteProfileString(const char* profile, const char* section, const char* entry, const char* value)
{
	FILE *fp, *fTemp;
	char TEMP[BUF_LEN];
	char buffer[BUF_LEN];
	char sec[BUF_LEN]/*, keyword[BUF_LEN]*/;
	int entryLen;
	int foundSection = 0, written = 0;
	char *p;

	if (profile == NULL || section == NULL || entry == NULL || entry[0] == '\0' || value == NULL)
		return 0;

	sprintf(sec, "[%s]", section);
	/* sprintf(keyword, "%s=", entry); */
	entryLen = strlen(entry);

	fp = fopen(profile, "rb");
	if (fp == NULL) {
		/* no profile exists, create it */
		fp = fopen(profile, "wb");
		if (fp == NULL)
			return 0;
		fprintf(fp, "###### %s ######\n"
					"%s\n"
					"%s=%s\n",
					profile,
					sec,
					entry, value
				);
		fclose(fp);
		return 1;
	}

	/* create a temprary file to store the profile content */
	sprintf(TEMP, "%s.tmp", profile);
	fTemp = fopen(TEMP, "wb");
	if (fTemp == NULL) {
		fclose(fp);
		return 0;
	}

	while (fgets(buffer, BUF_LEN, fp)) {
		if (IsBlankLine(buffer))
			continue;

		p = buffer;
		while (*p==' ' || *p=='\t') p++; /* skip the heading spaces */

		/* test if the line is a comment */
		if (*p == '#') {
			fprintf(fTemp, "%s", p);
			continue;
		}

		/* test section */
		if (!foundSection) {
			if (StartsWith(p, sec, 0)) {
				fputc('\n', fTemp);
				foundSection = 1;
			}
			fprintf(fTemp, "%s", p);
			continue;
		}

		/* a new section encounters, add the keyword */
		if (StartsWith(p, "[", 0)) {
			fprintf(fTemp, "%s=%s\n", entry, value);
			fprintf(fTemp, "\n%s", p);

			/* output all the remaining content */
			while (fgets(buffer, BUF_LEN, fp)) {
				fprintf(fTemp, "%s", buffer);
			}

			written = 1;
			break;
		}

		/* the keyword found, update it */
		if (StartsWith(p, (char*)entry, 0)) {
			char* pValue = p+entryLen;
			if (*pValue == '=' || *pValue == ' ' || *pValue == '\t') {
				int breakLoop = 0;
				while (!breakLoop && *pValue != '\0' && *pValue != '#' && *pValue != '\n' && *pValue != '\r') {
					switch (*pValue) {
					case ' ': case '\t':
						pValue++;
						break;
						
					case '=':
					default:
						breakLoop = 1;
						break;
					}
				}
				
				if (*pValue == '=') {
					fprintf(fTemp, "%s=%s\n", entry, value);
		
					/* output all the remaining content */
					while(fgets(buffer, BUF_LEN, fp)){
						fprintf(fTemp, buffer);
					}
		
					written = 1;
					break;
				}
			}
		}

		/* output a line */
		fprintf(fTemp, "%s",p);
	}

	/* the end the file encounters */
	if (!written) {
		if (!foundSection) {
			fprintf(fTemp, "%s\n", sec);
		}
		fprintf(fTemp, "%s=%s\n", entry, value);
	}

	fclose(fTemp);
	fclose(fp);

	/* rename */
	rename(TEMP, profile);
	return 1;
}

/******************************************************
 WriteProfileInt/WriteProfileUInt:
  write an integer entry to the profile
 Parameters:
  profile: the name of the profile. If the file doesn't
		   exists, a new file will be created.
  section: the name of a section. If no such section
		   exists, the section will be inserted into
		   the profile.
  entry:   the name of an entry to be added or updated.
  value:   the value of the entry.
 Return:
  1: successful
  0: any error occurrs
 ******************************************************/
int WriteProfileInt(const char* profile, const char* section, const char* entry, int value)
{
	char buf[20];
	sprintf(buf, "%d", value);
	return WriteProfileString(profile, section, entry, buf);
}
int WriteProfileUInt(const char* profile, const char* section, const char* entry, unsigned int value)
{
	char buf[20];
	sprintf(buf, "%u", value);
	return WriteProfileString(profile, section, entry, buf);
}

/******************************************************
 GetProfileString:
  get the value of an entry from the profile
 Parameters:
  profile: the name of the profile.
  section: the name of a section.
  entry:   the name of an entry to look for
  def:     the default value to set if the given entry
		   doesn't exist.
  ret:     the buffer to store the result
  size:    the size of buffer result buffer
 Return:
  0: any error occurrs
  >0: the number of bytes stored in the buffer
 ******************************************************/
int GetProfileString(const char* profile, const char* section, const char* entry, const char* def, char* ret, int size)
{
	FILE* fp;
	char buffer[BUF_LEN];
	char sec[BUF_LEN]/*, keyword[BUF_LEN]*/;
	int entryLen;
	int foundSection = 0;
	int retSize;

	if (profile == NULL || section == NULL || entry == NULL || entry[0] == '\0' || def == NULL || size <= 0)
		return 0;

	
	strncpy(ret, def, size); /* copy the default value */
	retSize = strlen(def);
	if (retSize > size) {
		retSize = size;
	}

	sprintf(sec, "[%s]", section);
	/* sprintf(keyword, "%s=", entry); */
	entryLen = strlen(entry);

	fp = fopen(profile, "rb");
	if (fp == NULL) {
		return retSize;
	}

	while (fgets(buffer, BUF_LEN, fp)) {
		if (!foundSection) {
			if (StartsWith(buffer, sec, 0)) {
				foundSection = 1;
			}
			continue;
		}

		/* a new section encounters, the entry not found */
		if (StartsWith(buffer, "[", 0))
			break;

		/* the entry found */
		if (StartsWith(buffer, (char*)entry, 0)) {
			char* pValue = buffer+entryLen;
			if (*pValue == '=' || *pValue == ' ' || *pValue == '\t') {
				/* look for delimiter '=' */
				int breakLoop = 0;
				while (!breakLoop && *pValue != '\0' && *pValue != '#' && *pValue != '\n' && *pValue != '\r') {
					switch (*pValue) {
					case ' ': case '\t':
						pValue++;
						break;
						
					case '=':
					default:
						breakLoop = 1;
						break;
					}
				}
				if (*pValue != '=') {
					continue;
				}
				pValue++;
				
				/* skip heading blank of value */
				while (*pValue != '\0' && *pValue != '\n' && *pValue != '\r' && *pValue != '#') {
					if (*pValue == ' ' || *pValue == '\t') {
						pValue++;
					} else {
						break;
					}
				}
				
				/* copy the value */
				retSize = 0;
				while (retSize < size && *pValue != '\0' && *pValue != '\n' && *pValue != '\r' && *pValue != '#') {
					*ret++ = *pValue++;
					retSize++;
				}
				
				/* skip ending blank of value */
				while (retSize > 0) {
					pValue = (ret-1);
					if (*pValue == ' ' || *pValue == '\t') {
						ret--;
						retSize--;
					} else {
						break;
					}
				}
	
				/* set the eos flag */
				if (retSize < size) {
					*ret = 0;
				}
				break;
			}
		}
	}
	fclose(fp);

	return retSize;
}

/******************************************************
 GetProfileInt/GetProfileUInt:
  get the value of an entry from the profile
 Parameters:
  profile: the name of the profile.
  section: the name of a section.
  entry:   the name of an entry to look for
  def:     the default value to set if the given entry
		   doesn't exist.
 Return:
  the value
 ******************************************************/
int GetProfileInt(const char* profile, const char* section, const char* entry, int def)
{
	char buf[40];
	int size;

	sprintf(buf, "%d", def);
	if ((size = GetProfileString(profile, section, entry, buf, buf, 39)) > 0) {
		buf[size] = '\0';
		return atoi(buf);
	}

	return def;
}
unsigned int GetProfileUInt(const char* profile, const char* section, const char* entry, unsigned int def)
{
	char buf[40];
	int size;

	sprintf(buf, "%u", def);
	if ((size = GetProfileString(profile, section, entry, buf, buf, 39)) > 0) {
		buf[size] = '\0';
		return strtoul(buf, NULL, 10);
	}

	return def;
}

typedef struct {
	int start;
	int len;
} SUB_STR_T;
static int isDeli(char ch, const char* deli)
{
	if (index(deli, ch) != NULL) return 1;
	return 0;
}
/******************************************************
 GetProfileStrings:
  get a series of values of an entry from the profile
 Parameters:
  profile: the name of the profileä.
  section: the name of a section.
  entry:   the name of an entry to look for
  ret:     the pointer to store the result
  deli:    the delimeter
 Return:
  0: any error occurrs
  >0: the number of values stored in the ret, which must be freed after using it
 ******************************************************/
int GetProfileStrings(const char* profile, const char* section, const char* entry, char*** ret, const char* deli)
{
	char buf[BUF_LEN+1] = {0};
	SUB_STR_T pos[MAX_ITEM];
	int count;
	int i;
	char* p;
	char** result = NULL;
	
	if (GetProfileString(profile, section, entry, "", buf, BUF_LEN) == 0 || buf[0] == '\0') {
		return 0; /* no entry */
	}
	if (deli == NULL || deli[0] == '\0') {
		/* no delimeter, only one item */
		result = (char**)malloc(sizeof(char*));
		if (result == NULL) {
			return 0;
		}
		*result = strdup(buf);
		if (*result == NULL) {
			free(result);
			return 0;
		}
		*ret = result;
		return 1;
	}
	
	/*split with deli */
	/*1. count substr number, and remember the postion of each substring */
	count = 0;
	p = buf;
	while (*p && count < MAX_ITEM) {
		while (*p && isDeli(*p, deli))p++;
		if (*p == '\0') break;
		
		pos[count].start = p - buf;
		pos[count].len = 1;
		p++;
		while (*p && !isDeli(*p, deli)) {
			p++;
			pos[count].len++;
		}
		count++;
		
		if (*p == '\0') break;
	}
	if (count == 0) {
		return 0;
	}
	
	/*2. set result */
	result = (char**)malloc(sizeof(char*) * count);
	if (result == NULL) {
		return 0;
	}
	for (i=0; i<count; i++) {
		result[i] = (char*)malloc(pos[i].len + 1);
		if (result[i] != NULL) {
			memcpy(result[i], buf+pos[i].start, pos[i].len);
			result[i][pos[i].len] = '\0';
		}
	}
	*ret = result;
	return count;
}

void FreeStrings(char** strs, int count)
{
	if (strs != NULL) {
		int i;
		for (i=0; i<count; i++) {
			if (strs[i] != NULL) {
				free(strs[i]);
			}
		}
		free(strs);
	}
}

/******************************************************
 GetProfileInts/GetProfileUInts:
  get a series of values of an entry from the profile, with the delimter ','
 Parameters:
  profile: the name of the profileä.
  section: the name of a section.
  entry:   the name of an entry to look for
  ret:     the pointer to store the result
 Return:
  0: any error occurrs
  >0: the number of values stored in the ret, which must be freed after using it
 ******************************************************/
int GetProfileInts(const char* profile, const char* section, const char* entry, int** ret)
{
	char** strs = NULL;
	int count;
	int* result = NULL;
	int i;
	
	count = GetProfileStrings(profile, section, entry, &strs, ", \t");
	if (count == 0) {
		return 0;
	}
	result = (int*)malloc(sizeof(int) * count);
	if (result == NULL) {
		FreeStrings(strs, count);
		return 0;
	}
	for (i=0; i<count; i++) {
		if (strs[i] != NULL) {
			result[i] = atoi(strs[i]);
		} else {
			result[i] = 0;
		}
	}
	FreeStrings(strs, count);
	
	*ret = result;
	return count;
}

int GetProfileUInts(const char* profile, const char* section, const char* entry, unsigned int** ret)
{
	char** strs = NULL;
	int count;
	unsigned int* result = NULL;
	int i;
	
	count = GetProfileStrings(profile, section, entry, &strs, ", \t");
	if (count == 0) {
		return 0;
	}
	result = (unsigned int*)malloc(sizeof(int) * count);
	if (result == NULL) {
		FreeStrings(strs, count);
		return 0;
	}
	for (i=0; i<count; i++) {
		if (strs[i] != NULL) {
			result[i] = strtoul(strs[i], NULL, 10);
		} else {
			result[i] = 0;
		}
	}
	FreeStrings(strs, count);
	
	*ret = result;
	return count;
}
