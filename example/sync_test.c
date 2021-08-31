/***************************************************************
 * 
 * @file:    sync_test.c
 * @author:  wilson
 * @version: 1.0
 * @date:    2021-08-21 22:45:42
 * @license: MIT
 * @brief:   同步方式测试例. 在模式下所有的日志记录在当前线程下执行.
 * 
 * 编译方式1(编译生成的程序, 执行时标准输出调试信息):
 * 		gcc sync_test.c nty_inproc_mq.c nty_logger.c -pthread
 * 
 * 编译方式2(编译生成的程序, 执行时标准输出无调试信息)
 * 		gcc sync_test.c nty_inproc_mq.c nty_logger.c -D NOLOGMSG -pthread
 * 
 ***************************************************************/

#include "nty_logger.h"

int main() {
	//[ 初始化同步日志器 ]
	ntylogger_init(SYNC_LOG);

	/*
		在执行路径下尝试创建logs/sync_module/[year]/[month]/sync-[day].log文件
		并在该文件中追加一条记录, 内容如下:
			[2021-08-24 15:27:46]--[sync_test.c:29:main()]--this is a complex sync test
	*/
	ntylog1("sync_module", "sync", "this is a complex %s test", "sync");
	/*
		在执行路径下尝试创建logs/sync_module/[year]/[month]/sync-[day].log文件
		并在该文件中追加一条记录, 内容如下:
			this is a simple sync test
	*/
	ntylog2("sync_module", "sync", "this is a simple %s test", "sync");

	//[ 销毁日志器 ]
	ntylogger_destroy();

	return 0;
}