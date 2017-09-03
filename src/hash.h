#ifndef _HASH_H
#define _HASH_H

static unsigned char crc_offset = 0;\

#define HASH_DEF_BEGIN(T)	\
typedef struct T T;\
struct T{\
	T *_next;\
	uint64_t _id;

#define HASH_DEF_END	};

//��ϣ�?��
#define HASH_DEF(T, total)	\
struct{	\
	T nodes[total], *index[total + 1];	\
	uint32_t slot, num;	\
}

//�̰߳�ȫ��ϣ�?��
#define HASHS_DEF(T, total)	\
struct{	\
	T nodes[total], *index[total + 1];	\
	uint32_t slot, num;	\
	pthread_rwlock_t lock;	\
}

#define CRC_OFF(k,l,h)     {h=crc_offset++;}
//��ϣ�㷨
#define HASH(k,l,h)	{uint8_t *_p=(uint8_t*)k;int _l=l;h=0;while(_l--)h=h*33+*(_p++);} 
//#define HASH(key, len, h){h = crc64(0, (unsigned char*)key, len);}i
//unsigned genCrc(unsigned char* data, int nLength);
//#define HASH(key, len, h){h = genCrc( (unsigned char*)key, len);}
//�õ���ϣ�������
#define HASH_NUM(table) (table).num
#define HASHS_NUM(table) (table).num

//��ʼ����ϣ��
#define HASH_INIT(table, total)	\
	memset(&(table), 0, sizeof(table));	\
	(table).slot = total;	\
	(table).index[total] = (table).nodes

#define HASHS_INIT(table, total)	\
	HASH_INIT(table, total);	\
	pthread_rwlock_init(&(table).lock, NULL)

//���ù�ϣ��
#define HASH_RESET(table)	\
	memset(&(table).index, 0, sizeof((table).index));\
	memset(&(table).nodes, 0, sizeof((table).nodes));\
	(table).num = 0;	\
	(table).index[(table).slot] = (table).nodes;

//�ͷŹ�ϣ��
#define HASHS_RELEASE(table) pthread_rwlock_destroy(&(table).lock);

//����
#define HASH_SEEK(table, id, node)	{	\
	int _idx = (id) % (table).slot;	\
	node = (table).index[_idx];	\
	while(node && (node)->_id != id)	\
		node = (node)->_next;\
}

#define HASHS_SEEK(table, id, node)	{	\
	int _idx;	\
	pthread_rwlock_rdlock(&(table).lock);	\
	_idx = (id) % (table).slot;\
	node = (table).index[_idx];\
	while(node && (node)->_id != id)\
		node = (node)->_next;\
	pthread_rwlock_unlock(&(table).lock);	\
}

//�õ�һ�����õĽڵ�
#define HASH_FREE_NODE(table, node)	{	\
	node = (table).index[(table).slot];\
	(node)->_next ? (table).index[(table).slot] = (node)->_next, (node)->_next = NULL : (table).index[(table).slot]++;	\
}

//assert((void*)(node) < (void*)(table).index);	\

//����
#define HASH_INSERT(table, id, node){	\
	void **_node = NULL;	\
	int _idx = (id) % (table).slot;	\
	node = (table).index[_idx];	\
	while(node && (node)->_id != id)	\
		_node = (void**)node, node = (node)->_next;	\
	if(!node){	\
		HASH_FREE_NODE(table, node);	\
		if(_node)	\
			*_node=(void*)node;	\
		else	\
			(table).index[_idx] = node;	\
		(node)->_id = id;	\
		(table).num++;	\
	}else{	\
		node = NULL;	\
	}	\
}

#define HASHS_INSERT(table, id, node){	\
	void **_node = NULL;	\
	int _idx;	\
	pthread_rwlock_wrlock(&(table).lock);	\
	_idx = (id) % (table).slot;	\
	node = (table).index[_idx];	\
	while(node && (node)->_id != id)	\
		_node = (void**)node, node = (node)->_next;	\
	if(!node){	\
		HASH_FREE_NODE(table, node);	\
		if(_node)	\
			*_node = (void*)node;	\
		else 	\
			(table).index[_idx] = node;	\
		(node)->_id = id;	\
		(table).num ++;	\
	}else{	\
		node = NULL;	\
	}	\
	pthread_rwlock_unlock(&(table).lock);	\
}

//ɾ��
#define HASH_DEL(table, id, node){	\
	void **_node = NULL;	\
	int _idx = (id) % (table).slot;	\
	node = (table).index[_idx];	\
	while(node && (node)->_id != id)	\
		_node = (void**)node, node = (node)->_next;	\
	if(node){	\
		if(_node)	\
			*_node = (void*)(node)->_next;	\
		else 	\
			(table).index[_idx] = (node)->_next;	\
		memset(node, 0, sizeof(*node));	\
		(node)->_next = (table).index[(table).slot];	\
		(table).index[(table).slot] = node;	\
		(table).num --;	\
	}	\
}

#define HASHS_DEL(table, id, node){	\
	void **_node = NULL;	\
	int _idx;	\
	pthread_rwlock_wrlock(&(table).lock);	\
	_idx = (id) % (table).slot;	\
	node = (table).index[_idx];	\
	while(node && (node)->_id != id)	\
		_node = (void**)node, node = (node)->_next;	\
	if(node){	\
		if(_node)	\
			*_node = (void*)(node)->_next;	\
		else 	\
			(table).index[_idx] = (node)->_next;	\
		memset(node, 0, sizeof(*node));	\
		(node)->_next = (table).index[(table).slot];	\
		(table).index[(table).slot] = node;	\
		(table).num --;	\
	}	\
	pthread_rwlock_unlock(&(table).lock);	\
}

//����
#define HASH_PASS(table, func, args, node){	\
	uint32_t _id;	\
	int _i,_ret,_count = 0, _num = (table).num;	\
	for(_i = 0; _i < (table).slot; _i++){	\
		node = (table).nodes + _i;	\
		if((node)->_id){	\
			if((_ret = func(node, args)) == -1){	\
				_id = (node)->_id;	\
				HASH_DEL(table, _id, node);	\
			}else if(_ret)	\
				break;	\
			if(++_count == _num)	\
				break;	\
		}	\
	}	\
}

#define HASHS_PASS(table, func, args, node){	\
	uint32_t _id;	\
	int _i, _ret, _count = 0, _num;	\
	pthread_rwlock_wrlock(&(table).lock);	\
	_count = (table).num;	\
	for(_i = 0; _i < (table).slot; _i++){	\
		node = (table).nodes + _i;	\
		if((node)->_id){	\
			if((_ret = func(node, args)) == -1){	\
				_id = (node)->_id;	\
				HASH_DEL(table, _id, node);	\
			}else if(_ret)	\
				break;	\
			if(++_count == _num)	\
				break;	\
			}	\
	}	\
	pthread_rwlock_unlock(&(table).lock);	\
}

#endif
