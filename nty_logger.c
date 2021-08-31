/***************************************************************
 * 
 * @file:    nty_logger.c
 * @author:  wilson
 * @version: 1.0
 * @date:    2021-08-24 14:27:43
 * @license: MIT
 * @brief:   日志记录器
 * 
 ***************************************************************/

#include "nty_logger.h"

#ifdef __cplusplus
extern "C" {
#endif

static void* log_wr_routine(void* arg);
static void sync_write(const char* log_cnt, const char* module_name, const char* proc_name);
static void async_write(const char* log_cnt, const char* module_name, const char* proc_name);
static int make_path(char* filepath, const char* module_name, const char* proc_name);


typedef struct LogMsg {
	char log_cnt[MAX_LOG_CNT_LEN];
	char module_name[MAX_MODNAME_LEN];
	char proc_name[MAX_PROCNAME_LEN];
} LogMsg;

typedef struct Logger {
	int log_way;		//[ 异步还是同步记录 ]
	int has_created;	//[ 确保变量 ]
	MessageQueue* mq;
	pthread_mutex_t wr_lock;	//[ 写日志锁 ]
	pthread_mutex_t state_lock;	//[ 状态机锁 ]
	pthread_t log_writer;
	int term_flag;		//[ log_writer线程结束标志 ]
} Logger;

//[ 单例对象 ]
static Logger g_logger = {0, 0, NULL, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, 0, 0};	

static void* log_wr_routine(void* arg) {
	for (;;) {
		LogMsg* msg = (LogMsg*)mq_tryread(g_logger.mq);
		if (msg == NULL) {
			if (g_logger.term_flag == 0){
				//[ 写日志线程不会很忙, 让出cpu ]
				usleep(1);
			} else {
				//[ 读完且退出flag on时, 退出 ]
				break;
			}
		} else {
			char filepath[1024] = {0};
			if (make_path(filepath, msg->module_name, msg->proc_name) == -1) {
#if LOGMSG
				printf("[nty_logger] failed to create log path\n");
#endif
				free(msg);
				continue;
			}
			pthread_mutex_lock(&g_logger.wr_lock);

			int fd = open(filepath, O_RDWR | O_CREAT | O_APPEND, 0777);
			if (fd == -1) {
#if LOGMSG
				printf("[nty_logger] failed to open log file\n");
#endif
				pthread_mutex_unlock(&g_logger.wr_lock);
				free(msg);
				continue;
			} 

			int len = strlen(msg->log_cnt);
			write(fd, msg->log_cnt, len);
			close(fd);
			pthread_mutex_unlock(&g_logger.wr_lock);
			free(msg);
		}
	}
}

int ntylogger_init(int log_way) {

	pthread_mutex_lock(&g_logger.state_lock);
	if (g_logger.has_created == 1) {
		pthread_mutex_unlock(&g_logger.state_lock);
		return -1;
	}
	if (log_way != SYNC_LOG)
		log_way = ASYNC_LOG;
	g_logger.log_way = log_way;

	if (log_way == ASYNC_LOG) {
		g_logger.mq = mq_init();
		if (g_logger.mq == NULL) {
			pthread_mutex_destroy(&g_logger.wr_lock);
			return -1;	
		}

		int ret = pthread_create(&g_logger.log_writer, NULL, log_wr_routine, NULL);
		if (ret != 0) {
			pthread_mutex_destroy(&g_logger.wr_lock);
			mq_destroy(g_logger.mq);
			return -1;
		}
	}
	g_logger.has_created = 1;
#if LOGMSG
	printf("[nty_logger] logger start working...\n");
#endif
	pthread_mutex_unlock(&g_logger.state_lock);
	return 0;
}

void write_full_log(const char* module_name, const char* proc_name, const char* filename, int lineNo, const char* funcname, const char* fmt, ...) {
	//[ 初始化时间对象, 可变参数对象 ]
	char msg[4096] = {0};
	char log_cnt[MAX_LOG_CNT_LEN] = {0};
	time_t rawtime = 0;
	struct tm* now = NULL;
	time(&rawtime);
	now = localtime(&rawtime);

	va_list ap;
	va_start(ap, fmt);
	vsprintf(msg, fmt, ap);
	va_end(ap);

	//[ 构造log日志的内容 ]
	snprintf(log_cnt, 4096, "[%04d-%02d-%02d %02d:%02d:%02d]--[%s:%d:%s()]--%s\n", 
								now->tm_year+1900, now->tm_mon+1, now->tm_mday,
								now->tm_hour, now->tm_min, now->tm_sec, filename, lineNo, funcname, msg);

	if (g_logger.log_way == SYNC_LOG) {
		sync_write(log_cnt, module_name, proc_name);
	} else {
		async_write(log_cnt, module_name, proc_name);
	}
}

void write_simple_log(const char* module_name, const char* proc_name, const char* fmt, ...) {
	char msg[4096] = {0};
	char log_cnt[MAX_LOG_CNT_LEN] = {0};
	time_t rawtime = 0;
	struct tm* now = NULL;
	time(&rawtime);
	now = localtime(&rawtime);

	va_list ap;
	va_start(ap, fmt);
	vsprintf(msg, fmt, ap);
	va_end(ap);

	//[ 构造log日志的内容 ]
	snprintf(log_cnt, 4096, "%s\n", msg);

	if (g_logger.log_way == SYNC_LOG) {
		sync_write(log_cnt, module_name, proc_name);
	} else {
		async_write(log_cnt, module_name, proc_name);
	}
}


void ntylogger_destroy() {
	pthread_mutex_lock(&g_logger.state_lock);
	if (g_logger.has_created == 0) {
		pthread_mutex_unlock(&g_logger.state_lock);
		return;
	}
	
	if (g_logger.log_way == ASYNC_LOG) {
		//[ 给100ms时间, 让子弹飞一会儿 ]
		usleep(100000);
		g_logger.term_flag = 1;
#if LOGMSG
		printf("[nty_logger] waiting for log writer thread quitting...\n");
#endif
		pthread_join(g_logger.log_writer, NULL);
		mq_destroy(g_logger.mq);
	}
	pthread_mutex_destroy(&g_logger.wr_lock);
	g_logger.has_created = 0;
#if LOGMSG
	printf("[nty_logger] quitted...\n");
#endif
	pthread_mutex_unlock(&g_logger.state_lock);
}



/**
 * @brief 	(1) 生成文件夹./logs/module/year/month  (2) 生成log日志文件名./logs/module/year/month/proc-day.log
 * 
 * @param filepath[out] 生成的log日志文件名(全路径)
 * @param module_name[in] 模块名, 用作路径生成
 * @param proc_name[in] 进程名, 用作日志文件名生成
 * @return 路径生成成功返回0, 失败返回-1
 */
static int make_path(char* filepath, const char* module_name, const char* proc_name) {
	/*
		要构造的filepath
		"./logs/module_name/2021/06/proc_name-27.log"
		即
		"./logs/[module_name]/[y_dir]/[m_dir]/[proc_name]-[day].log"

		使用mkdir()函数进行构造, 但是他只能一级级目录去构造, 无法mkdir -p
	*/
	char top_dir[1024] = LOG_ROOT_DIR;
	char module_dir[1024] = {0};
	char y_dir[1024] = {0};
	char m_dir[1024] = {0};

	time_t rawtime;
	struct tm* now = NULL;
	time(&rawtime);
	now = localtime(&rawtime);

	//[ 构成完整的log文件filename ]
	snprintf(filepath, 1024, "%s/%s/%04d/%02d/%s-%02d.log", LOG_ROOT_DIR, module_name, now->tm_year + 1900, now->tm_mon + 1, proc_name, now->tm_mday);

	//[ 准备每一级目录 ]
	sprintf(module_dir, "%s/%s", top_dir, module_name);				// ./logs/module
	sprintf(y_dir, "%s/%04d/", module_dir, now -> tm_year + 1900);	// ./logs/module/year
	sprintf(m_dir, "%s/%02d/", y_dir, now -> tm_mon + 1);			// ./logs/module/year/mon

	// 尝试进入./logs
	if (access(top_dir, F_OK) == -1) {	
		// 无法进入./logs, 创建之
		if (mkdir(top_dir, 0777) == -1) {
			fprintf(stderr, "create %s failed!\n", top_dir); return -1;
		} else if (mkdir(module_dir, 0777) == -1) { 
			fprintf(stderr, "%s, create %s failed!\n", top_dir, module_dir); return -1;
		} else if (mkdir(y_dir, 0777) == -1) {
			fprintf(stderr, "%s:create %s failed!\n", top_dir, y_dir); return -1;
		} else if (mkdir(m_dir, 0777) == -1) {
			fprintf(stderr, "%s:create %s failed!\n", top_dir, m_dir); return -1;
		}
	}
	//尝试进入./logs/module_name
	else if (access(module_dir, F_OK) == -1) {	
		// 无法进入"./logs/module_name", 创建之
		if (mkdir(module_dir, 0777) == -1) { 
			fprintf(stderr, "%s, create %s failed!\n", top_dir, module_dir); return -1;
		} else if (mkdir(y_dir, 0777) == -1) {
			fprintf(stderr, "%s:create %s failed!\n", top_dir, y_dir); return -1;
		} else if (mkdir(m_dir, 0777) == -1) {
			fprintf(stderr, "%s:create %s failed!\n", top_dir, m_dir); return -1;
		}
	}
	//尝试进入./logs/module_name/year
	else if (access(y_dir, F_OK) == -1) {
		if (mkdir(y_dir, 0777) == -1) {
			fprintf(stderr, "%s:create %s failed!\n", top_dir, y_dir); return -1;
		} else if (mkdir(m_dir, 0777) == -1) {
			fprintf(stderr, "%s:create %s failed!\n", top_dir, m_dir); return -1;
		}
	}
	else if (access(m_dir, F_OK) == -1) {
		if (mkdir(m_dir, 0777) == -1) {
			fprintf(stderr, "%s:create %s failed!\n", top_dir, m_dir); return -1;
		}
	}
	return 0;
}

static void sync_write(const char* log_cnt, const char* module_name, const char* proc_name) {
	
	char filepath[1024] = {0};
	//[ 创建存放log的文件夹目录 ]
	if (make_path(filepath, module_name, proc_name) == -1) {
#if LOGMSG
		printf("[nty_logger] failed to create log path\n");
#endif
		return;
	}

	//[ 写日志 ]
	pthread_mutex_lock(&g_logger.wr_lock);

	int fd = open(filepath, O_RDWR | O_CREAT | O_APPEND, 0777);
	if (fd == -1) {
#if LOGMSG
		printf("[nty_logger] failed to open log file\n");
#endif
		return;
	} 

	int len = strlen(log_cnt);
	write(fd, log_cnt, len);
	close(fd);
	pthread_mutex_unlock(&g_logger.wr_lock);
}


static void async_write(const char* log_cnt, const char* module_name, const char* proc_name) {
	LogMsg* logmsg = (LogMsg*)malloc(sizeof(LogMsg));
	if (logmsg == NULL) {
#if LOGMSG
		printf("[nty_logger] failed to alloc memory for logmsg\n");
		return;
#endif	
	}

	strncpy(logmsg->log_cnt, log_cnt, MAX_LOG_CNT_LEN);
	strncpy(logmsg->module_name, module_name, MAX_MODNAME_LEN);
	strncpy(logmsg->proc_name, proc_name, MAX_PROCNAME_LEN);

	mq_write(g_logger.mq, logmsg);
}


#ifdef __cplusplus
}
#endif