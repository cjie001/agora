/***************************************
 * This program is intended to provide
 * some functions to read & write .INI
 * format file in OS other than Windows
 *
 * FORMAT:
 * [section]
 * entry=value
 * any line begin with '#' is ignored
 *
 * int WriteProfileInt(const char* profile, const char* section, const char* entry, int value);
 * int WriteProfileString(const char* profile, const char* section, const char* entry, const char* value);
 * int GetProfileInt(const char* profile, const char* section, const char* entry, int def);
 * int GetProfileString(const char* profile, const char* section, const char* entry, const char* def, char* ret, int size);
 *
 * int WriteProfileUInt(const char* profile, const char* section, const char* entry, unsigned int value);
 * unsigned int GetProfileUInt(const char* profile, const char* section, const char* entry, unsigned int def);
 * int GetProfileStrings(const char* profile, const char* section, const char* entry, char*** ret, const char* deli);
 * void FreeStrings(char** strs, int count);
 * int GetProfileInts(const char* profile, const char* section, const char* entry, int** ret);
 * int GetProfileUInts(const char* profile, const char* section, const char* entry, unsigned int** ret);
 ***************************************/
#ifndef _PROFILE_H
#define _PROFILE_H

#ifdef __cplusplus
extern "C" {
#endif

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
  0: any error occurrs
  other: successful
 ******************************************************/
int WriteProfileInt(const char* profile, const char* section, const char* entry, int value);
int WriteProfileUInt(const char* profile, const char* section, const char* entry, unsigned int value);

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
  0: any error occurrs
  other: successful
 ******************************************************/
int WriteProfileString(const char* profile, const char* section, const char* entry, const char* value);

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
int GetProfileInt(const char* profile, const char* section, const char* entry, int def);
unsigned int GetProfileUInt(const char* profile, const char* section, const char* entry, unsigned int def);

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
int GetProfileString(const char* profile, const char* section, const char* entry, const char* def, char* ret, int size);

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
  >0: the number of values stored in the ret, [NOTE]which must be freed after using it
 ******************************************************/
int GetProfileStrings(const char* profile, const char* section, const char* entry, char*** ret, const char* deli);
void FreeStrings(char** strs, int count);

/******************************************************
 GetProfileInts/GetProfileUInts:
  get a series of values of an entry from the profile, with the delimter ',', ' ' or '\t'
 Parameters:
  profile: the name of the profileä.
  section: the name of a section.
  entry:   the name of an entry to look for
  ret:     the pointer to store the result
 Return:
  0: any error occurrs
  >0: the number of values stored in the ret, [NOTE]which must be freed after using it
 ******************************************************/
int GetProfileInts(const char* profile, const char* section, const char* entry, int** ret);
int GetProfileUInts(const char* profile, const char* section, const char* entry, unsigned int** ret);

#ifdef __cplusplus
}
#endif

#endif
