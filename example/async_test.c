/***************************************************************
 * 
 * @file:    async_test.c
 * @author:  wilson
 * @version: 1.0
 * @date:    2021-08-21 22:54:53
 * @license: MIT
 * @brief:   异步方式测试例. 创建一个专门用于记录日志的线程,
 * 		接受并记录其他线程(包括主线程)的日志信息.
 * 
 * 编译方式1(编译生成的程序, 执行时标准输出调试信息):
 * 		gcc async_test.c nty_inproc_mq.c nty_logger.c -pthread
 * 
 * 编译方式2(编译生成的程序, 执行时标准输出无调试信息)
 * 		gcc async_test.c nty_inproc_mq.c nty_logger.c -D NOLOGMSG -pthread
 * 
 ***************************************************************/


#include "nty_logger.h"

void* routine(void* arg) {
	/*
		在执行路径下尝试创建logs/async_module/[year]/[month]/async-[day].log文件
		并在该文件中追加一条记录, 内容如下:
			[2021-08-21 23:09:04]--[async_test.c:28:routine()]--this is a async test from sub thread
	*/
	ntylog1("async_module", "async", "this is a %s test from sub thread", "async");
}

int main() {

	//[ 初始化异步日志器 ]
	ntylogger_init(ASYNC_LOG);

	/*
		在执行路径下尝试创建logs/async_module/[year]/[month]/async-[day].log文件
		并在该文件中追加一条记录, 内容如下:
			[2021-08-21 23:09:04]--[async_test.c:36:main()]--this is a async test from main thread
	*/
	ntylog1("async_module", "async", "this is a %s test from main thread", "async");
	pthread_t t;
	pthread_create(&t, NULL, routine, NULL);

	pthread_join(t, NULL);

	//[ 销毁日志器 ]
	ntylogger_destroy();

	return 0;
}