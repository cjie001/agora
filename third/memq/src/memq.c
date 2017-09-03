#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include <syscall.h>
#include <unistd.h>
#include "memq.h"
#include "kfifo.h"

#define MAX_THREAD_NUM	100	//线程的最大个数
#define MAX_QUEUE_NAME 50	//队列名的最大长度
#define MAX_HOST_NAME	256	//机器名的最大长度
#define MAX_ADDR_NUM	256	//机器的最大个数
#define DEFAULT_QUEUE_PORT 11233	//队列的默认端口号

struct MEMQ{
	int		need_resolve;	//是否需要域名解析
	int		dns_time;		//域名解析时间
	int		addrs[MAX_ADDR_NUM];	//ip地址列表
	int		stats[MAX_ADDR_NUM];
	char		host[MAX_HOST_NAME];	//域名
	char		qname[MAX_QUEUE_NAME];	//队列名
	uint16_t	port;	//端口号
	uint16_t	qlen;	//qname的长度
	uint16_t	mc_num;	//机器的个数
	int		last_dns_time;	//上次域名解析的时间
	int		last_id;
	int		last_num;
	int		exptime;
	pthread_mutex_t	lock;	//互斥锁
	struct memcached_st *mc;	//memcached句柄
};

static int compare_int(int *a, int *b){
	if(*a > *b)
		return 1;
	else if(*a < *b)
		return -1;
	else return 0;
}

//解析ip地址
static int parse_host(char *ip, int *addrs, int num){
	char *p = ip;
	uint8_t n[4];
	int end = -1;
	int i;
	for(i = 0; i < 4; i ++){
		uint32_t ret = strtoul(p, &p, 0);
		if(ret >= 255)
			return -1;
		n[i] = ret;
		if(i < 3){
			if(*p != '.')
				return -1;
			p ++;
		}
	}
	if(*p == '-'){
		p ++;
		end = strtoul(p, &p, 0);
	}
	if(*p != 0)
		return -1;
	if(end == -1){
		addrs[0] = *(int*)n;
		return 1;
	}
	int m = 0;
	for(i = n[3]; m < num && i <= end; i ++){
		n[3] = i;
		addrs[m++] = *(int*)n;
	}
	return m;
}

//解析域名
//-1,域名解析失败
//其他值为ip的个数
static int resolve_host(char *host, int *addrs, int num, int *need_resolve){
	*need_resolve = 0;
	char ip[30];
	char *p = host;
	char *q = strchr(p, ',');
	int n = 0;
	while(1){
		int len;
		if(q == NULL)
			len = strlen(p);
		else len = q - p;
		memmove(ip, p, len);
		ip[len] = 0;
		int ret = parse_host(ip, addrs + n, num - n);
		if(ret < 0)
			break;
		n += ret;
		if(q == NULL)
			break;
		p = q + 1;
		q = strchr(p, ',');
	}
	if(n)
		return n;

	*need_resolve = 1;
	struct addrinfo hints = {0}, *addr = NULL, *paddr;
	hints.ai_family = AF_INET;
	hints.ai_protocol = IPPROTO_TCP;

	if(getaddrinfo(host, NULL, &hints, &addr)){ //域名解析失败
		return -1;
	}
	paddr = addr;
	int ip_num = 0;
	while(paddr){
		struct sockaddr_in *a = (struct sockaddr_in*)paddr->ai_addr;
		addrs[ip_num++] = a->sin_addr.s_addr;
		paddr = paddr->ai_next;
	}
	qsort(addrs, ip_num, sizeof(int), (void*)compare_int);
	freeaddrinfo(addr);
	return ip_num;
}

//检查是否需要域名解析
static int check_resolve(MEMQ *q){
	if(!q->need_resolve)	//不需要域名解析
		return 0;
	time_t now = time(NULL);
	if(now - q->last_dns_time < q->dns_time)
		return 0;
	pthread_mutex_lock(&q->lock);
	if(now - q->last_dns_time < q->dns_time){
		pthread_mutex_unlock(&q->lock);
		return 0;
	}
	q->last_dns_time = now;
	int addrs[MAX_ADDR_NUM];
	int need_resolve = 0;
	int num = resolve_host(q->host, addrs, MAX_ADDR_NUM, &need_resolve); //域名解析
	if(num <= 0){	//解析失败了
		pthread_mutex_unlock(&q->lock);
		return -1;
	}
	//检查ip地址是否有变化
	int is_changed = num != q->mc_num;
	if(!is_changed){
		int i;
		for(i = 0; i < num; i ++){
			if(q->addrs[i] != addrs[i]){
				is_changed = 1;
				break;
			}
		}
	}
	if(!is_changed){	//没有变化
		pthread_mutex_unlock(&q->lock);
		return 0;
	}
	memmove(q->addrs, addrs, num * sizeof(int));
	q->mc_num = num;
	memcached_servers_reset(q->mc);
	int i;
	for(i = 0; i < num; i ++){
		char ip[16];
		inet_ntop(AF_INET, addrs + i, ip, 16);
		memcached_server_add(q->mc, ip, q->port);
	}
	pthread_mutex_unlock(&q->lock);
	return 0;
}

//解析队列名和域名
static int parse_queue(const char *str, char *queue, char *host){
	*queue = 0;
	*host = 0;
	char *p = strchr(str, '@');
	if(p == NULL){
		strcpy(queue, str);
		return 0;
	}
	int len = p - str;
	memmove(queue, str, len);
	queue[len] = 0;
	str = p + 1;
	p = strchr(str, ':');
	if(p == NULL){
		strcpy(host, str);
	}else{
		len = p - str;
		memmove(host, str, len);
		host[len] = 0;
		return atoi(p + 1);
	}
	return 0;
}

//自定义哈希函数，用来实现向指定的服务器发请求
static inline uint32_t myhash(const char *key, size_t key_length, void *context){
	int i, n = 0;
	for(i = 0; i < key_length; i ++){
		n += (uint8_t)key[i];
	}
	return n;
}

//设置选项
int memq_set_option(MEMQ *q, int flag, uint64_t val){
	if(flag == MEMQ_OPTION_DNS_TIME){
		q->dns_time = val;
		return 0;
	}
	return memcached_behavior_set(q->mc, (memcached_behavior_t)flag, val);
}

//创建一个memq对象
MEMQ* memq_new(const char *str, int retry, double timeout){
	MEMQ *q = calloc(sizeof(MEMQ), 1);
	if(q == NULL)
		return NULL;
	
	q->port = parse_queue(str, q->qname, q->host); //解析队列名

	if(!*q->qname || !*q->host) //不正确
		goto LB_ERROR;

	q->qlen = strlen(q->qname);
	if(q->port == 0)
		q->port = DEFAULT_QUEUE_PORT;
	int need_resolve = 0;
	int num = resolve_host(q->host, q->addrs, MAX_ADDR_NUM, &need_resolve);
	if(num <= 0)	//域名解析失败
		goto LB_ERROR;
	if(num > 0){
		q->need_resolve = need_resolve;
		q->dns_time = 60;
		q->last_dns_time = time(NULL);
		q->mc_num = num;
	}
	pthread_mutex_init(&q->lock, NULL);

	//初始化memcached
	q->mc = memcached_create(NULL);
	if(q->mc == NULL)
		goto LB_ERROR;

	if(timeout == 0)
		timeout = 2000;
	else timeout *= 1000;
	
	memcached_behavior_set(q->mc, MEMCACHED_BEHAVIOR_RETRY_TIMEOUT, 5);	//失败重试时间
	memcached_behavior_set(q->mc, MEMCACHED_BEHAVIOR_AUTO_EJECT_HOSTS, 1);	//失败后不再使用该机器
	memcached_behavior_set(q->mc, MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT, 1000);	//连接超时
	memcached_behavior_set(q->mc, MEMCACHED_BEHAVIOR_POLL_TIMEOUT, timeout);	//读取超时

	hashkit_st *h = (void*)memcached_get_hashkit(q->mc);
	h->base_hash.function = myhash;
	h->distribution_hash.function = myhash;
	memcached_behavior_set(q->mc, MEMCACHED_BEHAVIOR_HASH, MEMCACHED_HASH_CUSTOM);

	int i;
	for(i = 0; i < num; i ++){
		char ip[16];
		inet_ntop(AF_INET, q->addrs + i, ip, 16);
		memcached_server_add(q->mc, ip, q->port);
	}
	q->last_id = -1;

	return q;
LB_ERROR:
	if(q->mc)
		memcached_free(q->mc);
	free(q);
	return NULL;
}

//得到错误信息
char* memq_error(MEMQ *q, memcached_return_t rc){
	return (char*)memcached_strerror(q->mc, rc);
}

//释放memq
void memq_free(MEMQ *p){
	memcached_free(p->mc);
	free(p);
}

//打印状态信息
int memq_stats(MEMQ *q){
	char buf[200];
	int i, pos = 0;
	pos += sprintf(buf + pos, "%ld\t", syscall(SYS_gettid));
	for(i = 0; i < q->mc_num; i ++){
		pos += sprintf(buf + pos, "%d:%d\t", i, q->stats[i]);
	}
	buf[pos - 1] = '\n';
	printf("%s", buf);
	return 0;
}

//读队列
int memq_get(MEMQ *q, char **valp){
	check_resolve(q);
	size_t vlen = 0;
	uint32_t flag;
	char *v = NULL;

	int ip_num = q->mc_num;
	int tid = syscall(SYS_gettid) % ip_num;

	memcached_return_t rc;

	char ch = '0' + (q->last_id == -1 ? tid : q->last_id);
	int i;
	for(i = 0; i < ip_num; i ++){
		v = memcached_get_by_key(q->mc, &ch, 1, q->qname, q->qlen, &vlen, &flag, &rc);
		if(v)
			break;
		ch ++;
	}
	if((*valp = v) == NULL)
		return 0;
	int server_id = (ch - '0') % ip_num;
	if(i){
		q->last_id = server_id;
		q->last_num = 1;
	}else if(q->last_id != -1){
		q->last_num ++;
		if(q->last_num == 100){
			q->last_id = (server_id + 1) % ip_num;
			if(q->last_id == tid)
				q->last_id = -1;
		}
	}
	q->stats[server_id] ++;
	return vlen;
}

//向指定的服务器写队列
int memq_set_key(MEMQ *p, char *v, int vlen, char *key){
	check_resolve(p);
	memcached_return_t rc;
	int ip_num = p->mc_num;
	if(ip_num == 0)
		return -1;
	int i;
	char ch = '0' + myhash(key, strlen(key), NULL) % ip_num;
	for(i = 0; i < ip_num; i ++){
		rc = memcached_set_by_key(p->mc, &ch, 1, p->qname, p->qlen, v, vlen, p->exptime, 0);
		if(rc == MEMCACHED_SUCCESS)
			break;
		ch ++;
	}
	return rc;
}

//写队列
int memq_set(MEMQ *p, char *v, int vlen){
	check_resolve(p);
	memcached_return_t rc;
	int ip_num = p->mc_num;
	if(ip_num == 0)
		return -1;
	int i;
	char ch = '0' + rand() % ip_num;
	for(i = 0; i < ip_num; i ++){
		rc = memcached_set_by_key(p->mc, &ch, 1, p->qname, p->qlen, v, vlen, p->exptime, 0);
		if(rc == MEMCACHED_SUCCESS)
			break;
		ch ++;
	}
	return rc;
}

//删除整个队列
int memq_del(MEMQ *q){
	return memcached_delete(q->mc, q->qname, q->qlen, 0);
}

//修改队列的名字
void memq_set_key64(MEMQ *p, uint64_t key){
	p->qlen = sprintf(p->qname, "%lu", key);
}

//设置数据过期时间
void memq_set_exptime(MEMQ *p, int exptime){
	if(exptime < 0) exptime = 0;
	p->exptime = exptime;
}
#define THREAD_NUM	4

typedef struct{
	char *name;	//队列名
	void *queue;	//内存队列
	int running;	//是否正在运行
	int thread_num;	//读队列的线程数
	pthread_t pids[MAX_THREAD_NUM];	//线程的pid
}Dispatch;

//读取线程
static void* call_read_thread(void *args){
	Dispatch *dis = (Dispatch*)args;
	MEMQ *from = memq_new(dis->name, 0, 0);
	if(from == NULL)
		return NULL;
	while(dis->running){
		char *v = NULL;
		int size = memq_get(from, &v);
		if(v == NULL){
			usleep(100000);
			continue;
		}
		if(size < MAX_QUEUE_ITEM_SIZE)
			dfifo_put(dis->queue, 0, size, v, 1);
		free(v);
	}
	memq_free(from);
	return NULL;
}

//创建读队列的dispatch
void* memq_run(char *name, int thread_num, int queue_size_mb){
	char queue[MAX_QUEUE_NAME], host[MAX_HOST_NAME];
	parse_queue(name, queue, host);
	if(!*queue || !*host)	//name格式不正确
		return NULL;
	int addrs[MAX_ADDR_NUM];
	int need_resolve;
	int num = resolve_host(host, addrs, MAX_ADDR_NUM, &need_resolve);
	if(num <= 0)
		return NULL;
	if(thread_num == 0)
		thread_num = num;
	if(thread_num > MAX_THREAD_NUM)
		thread_num = MAX_THREAD_NUM;
	int i;
	Dispatch *dis = malloc(sizeof(*dis));
	assert(dis);
	size_t min_size = (size_t)MAX_QUEUE_ITEM_SIZE << 1;
	size_t max_size = (size_t)MAX_QUEUE_ITEM_SIZE << 8;
	size_t queue_size = (size_t)queue_size_mb << 20;
	if(queue_size < min_size)
		queue_size = min_size;
	else if(queue_size > max_size)
		queue_size = max_size;
	dis->name = name;
	dis->queue = dfifo_alloc(queue_size, NULL);
	assert(dis->queue);
	dis->running = 1;
	dis->thread_num = thread_num;
	for(i = 0; i < thread_num; i ++){
		pthread_create(dis->pids + i, NULL, call_read_thread, dis);
	}
	return dis;
}

//停止服务
void memq_stop(void *h){
	Dispatch *dis = (Dispatch*)h;
	dis->running = 0;
	int i;
	for(i = 0; i < dis->thread_num; i ++){
		pthread_join(dis->pids[i], NULL);
	}
	dfifo_put_end(dis->queue);
}

//得到一个请求
int memq_get_request(void *h, char *buf, int max_len, double timeout){
	Dispatch *dis = (Dispatch*)h;
	uint32_t cmd, len = max_len;
	int ret = dfifo_get(dis->queue, &cmd, &len, buf, timeout);
	return ret ? ret : len;
}

//得到一个请求,timeout为超时时间
void* memq_request2(void *h, double timeout){
	Dispatch *dis = (Dispatch*)h;
	char *data = malloc(MAX_QUEUE_ITEM_SIZE + 1);
	assert(data);
	uint32_t cmd, len = MAX_QUEUE_ITEM_SIZE;
	int ret = dfifo_get(dis->queue, &cmd, &len, data, timeout);
	if(ret < 0){	//没数据
		free(data);
		return NULL;
	}
	data[len] = 0;
	data = realloc(data, len + 1);
	assert(data);
	return data;
}

//得到一个请求，需要手动调用free释放内存
void* memq_request(void *h){
	return memq_request2(h, -1);
}
