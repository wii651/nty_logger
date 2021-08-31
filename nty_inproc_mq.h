/***************************************************************
 * 
 * @file:    nty_inproc_mq.h
 * @author:  wilson
 * @version: 1.0
 * @date:    2021-08-21 11:17:52
 * @license: MIT
 * @brief:   线程间消息队列组件
 * 	编译时需增加-pthread选项
 * 
 ***************************************************************/

#ifndef _NTY_INPROC_MQ_H_ 
#define _NTY_INPROC_MQ_H_ 

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MessageQueue		MessageQueue;

#define LOCK_OCCUPIED		1
#define LOCK_FREE			0

/* ============================ API ============================ */

/**
 * @brief 构建并初始化一个消息队列, 返回其handler
 * 
 * @return MessageQueue* -- 成功返回消息队列handler, 失败返回NULL
 */
MessageQueue* mq_init();

/**
 * @brief 向消息队列中添加消息
 * 
 * @param mq[in] 消息队列handler
 * @param msg_cnt[in] 消息内容, 需为堆空间数据
 * @return int -- 0 on success, -1 otherwise
 */
int mq_write(MessageQueue* mq, void* msg_cnt);

/**
 * @brief 非阻塞方式读取队列中消息
 * 
 * @param mq[in] 消息队列handler
 * @return void* -- 读取成功返回消息指针(其资源管理消息队列不再负责), 失败返回NULL
 */
void* mq_tryread(MessageQueue* mq);

/**
 * @brief 阻塞方式读取队列中消息
 * 
 * @param mq[in] 消息队列handler
 * @return void* -- 返回获取的消息指针(其资源管理消息队列不再负责)
 */
void* mq_read(MessageQueue* mq);

/**
 * @brief 销毁消息队列并释放其中剩余的消息资源
 * 
 * @param mq[in] 消息队列handler
 */
void mq_destroy(MessageQueue* mq);

#ifdef __cplusplus
}
#endif

#endif	// _NTY_INPROC_MQ_H_