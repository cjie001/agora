#ifndef _MKDIRS_H
#define _MKDIRS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 创建一个子目录，如果其父目录不存在，先创建其父目录
 * 参数：
 *  path: 路径名
 *        [要求]路径分隔符统一为'/'
 * 返回：
 *  1: 成功，0: 失败
 */
int mkdirs(char* path);

#ifdef __cplusplus
}
#endif

#endif

