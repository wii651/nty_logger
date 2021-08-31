/***************************************************************
 * 
 * @file:    nty_inproc_mq.c
 * @author:  wilson
 * @version: 1.0
 * @date:    2021-08-24 14:18:24
 * @license: MIT
 * @brief:	 线程间消息队列组件
 * 
 ***************************************************************/

#include "nty_inproc_mq.h"

#ifdef __cplusplus
extern "C" {
#endif

//[ 获取锁 ]
static inline void onlock(int* lock) {
	while (__sync_lock_test_and_set(lock, LOCK_OCCUPIED)) {
		usleep(10);
	}
}

//[ 尝试获取锁, 成功返回true, 未获取锁返回false ]
static inline int trylock(int* lock) {
	return (__sync_lock_test_and_set(lock, LOCK_OCCUPIED)) == LOCK_FREE;
}

//[ 释放锁 ]
static inline void offlock(int* lock) {
	__sync_lock_release(lock);
}

typedef struct MsgNode {
	struct MsgNode* next;
	void* user_data;
} MsgNode;

struct MessageQueue {
	struct MsgNode* head;
	struct MsgNode* tail;
	int op_lock;
	sem_t sem;
};

MessageQueue* mq_init() {
	MessageQueue* mq = (MessageQueue*)malloc(sizeof(MessageQueue));
	if (mq == NULL) return NULL;

	mq->head = NULL;
	mq->tail = NULL;
	mq->op_lock = 0;
	sem_init(&mq->sem, 0, 0);

	return mq;
}

int mq_write(MessageQueue* mq, void* msg_cnt) {
	MsgNode* node = (MsgNode*)malloc(sizeof(MsgNode));
	if (node == NULL) return -1;

	node->next = NULL;
	node->user_data = msg_cnt;
	onlock(&mq->op_lock);

	if (mq->head == NULL) {
		mq->head = node;
		mq->tail = node;
	} else {
		mq->tail->next = node;
		mq->tail = node;
	}

	offlock(&mq->op_lock);
	sem_post(&mq->sem);	//[ 信号量++ ]

	return 0;
}

void* mq_tryread(MessageQueue* mq) {

	MsgNode* node = NULL;
	void* msg_cnt = NULL;
	int ret = sem_trywait(&mq->sem);
	if (ret == 0) {		//[ 收到信号了, 信号量-- ]
		onlock(&mq->op_lock);
		if (mq->head == NULL)
			//[ 无节点, 直接返回 ]
			return NULL;
		else if (mq->head == mq->tail) {
			//[ 只有一个节点 ]
			node = mq->head;
			mq->head = NULL;
			mq->tail = NULL;
		} else {
			//[ 大于一个节点 ]
			node = mq->head;
			mq->head = mq->head->next;
		}
		offlock(&mq->op_lock);
		msg_cnt = node->user_data;
		free(node);
	}
	return msg_cnt;
}

void* mq_read(MessageQueue* mq) {
	MsgNode* node = NULL;
	void* msg_cnt = NULL;
	sem_wait(&mq->sem);
	onlock(&mq->op_lock);

	if (mq->head == NULL)
		//[ 无节点, 直接返回 ]
		return NULL;
	else if (mq->head == mq->tail) {
		//[ 只有一个节点 ]
		node = mq->head;
		mq->head = NULL;
		mq->tail = NULL;
	} else {
		//[ 大于一个节点 ]
		node = mq->head;
		mq->head = mq->head->next;
	}
	offlock(&mq->op_lock);
	msg_cnt = node->user_data;
	free(node);
	return msg_cnt;

}

void mq_destroy(MessageQueue* mq) {
	sem_destroy(&mq->sem);
	MsgNode* node = mq->head;
	MsgNode* next = NULL;
	while (node) {
		if (node->user_data) {
			free(node->user_data);
		}
		next = node->next;
		free(node);
		node = next;
	}
	free(mq);
}

#ifdef __cplusplus
}
#endif