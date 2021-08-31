/***************************************************************
 * 
 * @file:    nty_logger.h
 * @author:  wilson
 * @version: 1.0
 * @date:    2021-08-21 22:34:09
 * @license: MIT
 * @brief:   日志记录器
 * 	默认输出调试信息, 当编译时增加-D NOLOGMSG时, 不输出调试信息
 * 
 ***************************************************************/
#ifndef _NTY_LOGGER_H_ 
#define _NTY_LOGGER_H_ 

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#include "nty_inproc_mq.h"

//[ 日志根目录地址 ]
#define LOG_ROOT_DIR		"./logs"

#define MAX_LOG_CNT_LEN		4096
#define MAX_MODNAME_LEN		1024
#define MAX_PROCNAME_LEN	1024

#define ASYNC_LOG		1
#define SYNC_LOG		0

#ifdef NOLOGMSG
    #define LOGMSG 0
#else
    #define LOGMSG 1
#endif

/* ============================ API ============================ */

/**
 * @brief 初始化日志器(该日志器全局单例). 
 * 当该函数执行成功后, 在执行wlogger_destroy()之前, 无法再次初始化日志器.
 * #此函数线程安全#
 * 
 * @param log_way[in] 该传参为SYNC_LOG(0)时, 表示同线程下同步日志记录方式;
 * 该传参为ASYNC_LOG(1)时, 表示开启单独的线程日志异步记录方式
 * @return int -- 0 on success, -1 otherwise
 */
int ntylogger_init(int log_way);


/**
 * @brief 将完整的信息输出到日志文件
 * #此函数线程安全#
 * 
 * @param module_name[in] 当前模块名, 用于确定日志文件路径
 * @param proc_name[in] 当前进程名, 用于确定写入的日志文件名称
 * @param x...[in] 日志信息的格式化字符串, 形如["%s:%d", "score", 127]
 */
#define ntylog1(module_name, proc_name, x...) \
		do { \
			write_full_log(module_name, proc_name, __FILE__, __LINE__, __FUNCTION__, ##x); \
		} while(0)

/**
 * @brief 将简单的信息输出到日志文件
 * #此函数线程安全#
 * 
 * @param module_name[in] 当前模块名, 用于确定日志文件路径
 * @param proc_name[in] 当前进程名, 用于确定写入的日志文件名称
 * @param x...[in] 日志信息的格式化字符串, 形如["%s:%d", "score", 127]
 */
#define ntylog2(module_name, proc_name, x...) \
		do { \
			write_simple_log(module_name, proc_name, ##x); \
		} while(0)

/**
 * @brief 销毁日志器并释放相关资源. 
 * [同步方式]: 执行该函数将立刻销毁当前日志器, 停止日志记录;
 * [异步方式]: 执行该函数后, 再等100ms, 正式发出销毁日志器请求.
 * 此后, 日志线程会处理完所有剩余的日志队列中的消息, 然后正式释放.
 * #此函数线程安全#
 */
void ntylogger_destroy();


void write_full_log(const char* module_name, const char* proc_name, const char* filename, int lineNo, const char* funcname, const char* fmt, ...);
void write_simple_log(const char* module_name, const char* proc_name, const char* fmt, ...);


#ifdef __cplusplus
}
#endif

#endif	// _NTY_LOGGER_H_
