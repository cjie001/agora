#include <stdio.h>
#include <string.h>
#ifndef WIN32
#include <sys/types.h>
#include <sys/stat.h>
#else
#include <direct.h>
#endif
#include <errno.h>

/**
 * 创建一个子目录，如果其父目录不存在，先创建其父目录
 * 参数：
 *  path: 路径名
 *        [要求]路径分隔符统一为'/'
 * 返回：
 *  1: 成功，0: 失败
 */
int mkdirs(char* path)
{
	int len;
	int hasTailSlash = 0;
	char* fatherSlash;
	int createFatherOK;
	
	if (path == NULL || path[0] == '\0') {
		return 0;
	}
	if (path[0] == '/' && path[1] == '\0') {
		return 1;
	}
	
#ifndef WIN32
	if (!mkdir((const char*)path, 0755) || errno == EEXIST) {
#else
	if (!_mkdir((const char*)path) || errno == EACCES || errno == EEXIST) {
#endif
		/* 直接创建子目录成功 */
		return 1;
	}
	
	/* 暂时去掉结尾的'/' */
	len = strlen(path);
	if (path[len-1] == '/') {
		hasTailSlash = 1;
		path[len-1] = '\0';
	}
	
	/* 找父目录的'/'，如果找到暂时去掉它 */
	fatherSlash = strrchr(path, '/');
	if (fatherSlash == NULL) {
		/* 没有找到，恢复路径内容，返回 */
		if (hasTailSlash) {
			path[len-1] = '/';
		}
		return 0;
	}
	
	/* 暂时去掉父路径结尾的'/'，然后创建父目录 */
	*fatherSlash = '\0';
	createFatherOK = mkdirs(path);
	
	/* 恢复去掉的'/' */
	*fatherSlash = '/';
	if (hasTailSlash) {
		path[len-1] = '/';
	}
	
	if (createFatherOK) {
#ifndef WIN32
		if (!mkdir((const char*)path, 0755)) {
#else
		if (!_mkdir((const char*)path) || errno == EACCES) {
#endif
			return 1; /* 创建子目录成功 */
		}
	}
	
	return 0;
}

