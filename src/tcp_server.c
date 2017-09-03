/*******************************************
   Author        : Jun Zhang
   Email         : ewalker.zj@gmail.com
   Last Modified : 2017-06-20 18:11
   Filename      : tcp_server.c
   Description   : 
*******************************************/

#include "tcp_server.h"
#include "timer.h"


static WORKER *worker_threads;
static SERVER *server;
static QUEUE_WORKER *queue_threads;
static CONF conf = {0};


//static HASHS_DEF(LogInfo, MAX_TYPE_NUM) table;
static LOGTABLE table;

// dump data, use # replace 0
int func(const char *data, int size){
	char buffer[4096];
	int i;
	int start = 0;
	bool ret = false;
	for( i = 0; i < size; i++){
		if(data[i] == 0){
			if(ret){
				memcpy(buffer, data + start, i - start);
				buffer[i-start] = 0;
				printf("%s", buffer);
				ret = false;
			}
			printf("#");
		}
		else{
			if(!ret){
				ret = true;
				start = i;
			}
		}
	}
	if(ret){
		memcpy(buffer, data + start, size - start);
		buffer[size - start] = 0;
		printf("%s", buffer);
	}
}



void init_table()
{
	int i = 0;
	while(1)
	{
		char type_name[256];
		char type_queue[256];
		char tmp[256];
		sprintf(tmp, "type%d_name", i);
		GetProfileString(conf.conf_name, "global", tmp, "", type_name, 256);
		if(*type_name == 0)
		{
			break;
		}

		sprintf(tmp, "type%d_queue", i);
		GetProfileString(conf.conf_name, "global", tmp, "", type_queue, 256);

		LogInfo *info;
		int type_len = strlen(type_name);
		uint32 h;
		HASH(type_name, type_len, h);
		HASHS_INSERT(table, h, info);
		if(info)
		{
			strcpy(info -> type, type_name);
			strcpy(info -> queue_info, type_queue);
			slog_write(LL_WARNING, "%s\t%s", type_name, info -> queue_info);
		}
		i++;
	}
}


static Memcache* get_mcq(QUEUE_WORKER *thread, LogInfo *info){
	int i;
	Memcache *mem;
	for(i = 0; i < thread->mem_num; i ++){
		mem = thread->mems + i;
		if(strcmp(mem->queue_info, info->queue_info) == 0){
			break;
		}
	}
	if(i == thread->mem_num){
		mem = thread->mems + thread->mem_num ++;
		strcpy(mem->queue_info, info->queue_info);
		mem->memq = NULL;
	}
	if(mem->memq == NULL){
		mem->memq = memq_new(info->queue_info, INT_MAX, 10);
		if(mem->memq == NULL){
			return NULL;
		}
	}
	return mem;
}



//写入到mcq
int send_queue_log(QUEUE_WORKER *thread, LogHead *head, LogInfo *info){

	struct  timeval  start;
	struct  timeval  end;
	unsigned long timer;	

//#ifdef DEBUG_TIME
//		struct timespec sstart, send;
//		long long secdiff = 0, nsecdiff = 0;
//		clock_gettime(CLOCK_REALTIME, &sstart);
//		char timebuf[32];
//		memcpy(timebuf, head->log + head->size - 32, 31);
//		timebuf[31] = 0;
//		struct timespec *in = (struct timespec*)timebuf;
//#endif
//	char sendbuf[4096];
//	memcpy(sendbuf, head->log, head->size);
//	memcpy(sendbuf+4048, timebuf, 47);
//	sendbuf[4095] =  0;	

	Memcache *mem = get_mcq(thread, info);
	if(mem == NULL){
		return -1;
	}
	
	gettimeofday(&start,NULL);
	memcached_return_t rc = memq_set(mem->memq, head->log, head->size);
	gettimeofday(&end,NULL);
    timer = 1000000 * (end.tv_sec-start.tv_sec)+ end.tv_usec-start.tv_usec;
    if(timer > 1000000)
    {
    	//slog_write(LL_DEBUG, "set memcached delay %ld us", timer);
		memq_free(mem->memq);
		mem->memq = NULL;
    }


//#ifdef DEBUG_TIME
//		clock_gettime(CLOCK_REALTIME, &send);
//		secdiff = diff(sstart, send).tv_sec;
//		nsecdiff = diff(sstart, send).tv_nsec;
//		long long queue_diff = secdiff*1000 + nsecdiff/1000000; 
//		secdiff = diff(*in, send).tv_sec;
//		nsecdiff = diff(*in, send).tv_nsec;
//		long long total_diff = secdiff*1000 + nsecdiff/1000000; 
//		slog_write(LL_DEBUG, "total: %lld ms, queue: %lld ms, logdata:", total_diff, queue_diff, head->log);
//#endif

	if(rc != MEMCACHED_SUCCESS){
		slog_write(LL_WARNING, "set mcq error");
		if(rc == MEMCACHED_CONNECTION_FAILURE){ //连接失败，需要重连
			memq_free(mem->memq);
			mem->memq = NULL;
		}
		return -1;
	}else{
		//slog_write(LL_DEBUG, "set mcq %s\t%s", head->type, mem->queue_info);
	}
	return 0;
}


int _set_nonblocking(int fd)
{
	int flag = fcntl(fd, F_GETFL, 0);
	if( -1 == flag )
	{
		return -1;
	}

	return fcntl(fd, F_SETFL, flag | O_NONBLOCK);
}
inline void safe_close(int fd){
	int ret = close(fd);
	while(ret != 0){
		if(errno != EINTR || errno == EBADF)
			break;
		
		ret = close(fd);
	}
}


static int open_server_socket(const char *ip, short port, int backlog)
{
	printf("open server socket..................\n");
	int fd = -1;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd < 0)
	{
		sprintf(stderr, "open server socket failed, errno: %d %m\n", errno);
		return -1;
	}
	
	//unsigned long no_blocking = 1;
	//ioctl(fd, FIOBIO, &no_blocking);
	int flags = fcntl(fd, F_GETFL, 0);

	if( fcntl(fd, F_SETFL, flags|O_NONBLOCK) < 0 )
	{
		sprintf(stderr, "failed to set nonblocking IO, errno: %d %m\n", errno);
		safe_close(fd);
		return -1;
	}

	struct timeval tv = {0};
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv));
	setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char*)&tv, sizeof(tv));

	int flag_reuseaddr = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&flag_reuseaddr, sizeof(flag_reuseaddr));


	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);
	memset(&addr, 0, addrlen);

	if(ip == NULL)
		printf("ip: %s, port: %d\n", ip, port);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = (ip == NULL) ? INADDR_ANY : inet_addr(ip);

	if(bind(fd, (const struct sockaddr*)&addr, sizeof(addr)) != 0)
	{
		sprintf(stderr, "failed to bind socket on port: %d, errno: %d %m\n", port, errno);
		safe_close(fd);
		return -1;  
	}

	if(listen(fd, backlog) != 0)
	{
		printf("can not listen on port: %d, errno: %d, %s\n", port, errno, strerror(errno));
		safe_close(fd);
		return -1;
	}


	return fd;
}


CONN_LIST* init_conn_list(uint32 size){
	CONN_LIST *connlist = NULL;

	uint32 len = sizeof(CONN_LIST) + sizeof(CONN)*(size + 1);
	connlist = (CONN_LIST*)malloc(len);

	if(connlist == NULL)
		return NULL;

	memset(connlist, 0, len);

	connlist -> head = &connlist->list[0];
	connlist -> tail = &connlist->list[size];

	int i;
	for(i = 0; i < size; i++){
		connlist -> list[i].id = i;
		connlist -> list[i].next = &connlist->list[i+1];
	}

	connlist -> list[size].id = size;
	connlist -> list[size].next = NULL;
	
	return connlist;
}

void close_conn(WORKER *worker, CONN *c){
	safe_close(c->fd);
	PUT_CONN_ITEM(worker->conns, c);
}

static void worker_receive_process(int fd, int which, void *args){

	WORKER *worker = (WORKER*)args;
	uint32 id;
	if(read(fd, &id, 4) != 4)
	{
		slog_write(LL_WARNING, "worker receive threa read pipe failed, tid: %d, %s", worker -> id, strerror(errno));
		return;
	}

	//slog_write(LL_DEBUG, "receive thread, tid: %d, conn_id: %d", worker -> id, id);
	
	// worker recv setting
	//struct timeval tv = {0};
	//tv.tv_sec = 3;
	//tv.tv_usec = 0;
	//setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv));
	//
	//int nRecvBuf = 32*1024;
	//setsockopt(worker->conns->list[id].fd, SOL_SOCKET, SO_RCVBUF, (const char*)&nRecvBuf, sizeof(int));


	int start = 0, end = 0, read_len, write_len = 0;
	int ret, data_len;
	char head_tmp[25];
	char log_tmp[MAX_LOG_LEN + sizeof(LogHead)];
	bool is_empty = true, peer_close = false;
	size_t remain_size;
	int idx = 0;
	
	// for hash and squeue
	unsigned int type_len, h;
	unsigned int queue_set_size;
	char *queue_set_data;
	QUEUE_WORKER *queue_writer;

	char *buf = worker->conns->list[id].buf;
	
	while(1)
	{
		
		// init read and write state
		read_len = 0;
		write_len = 0;
		
		while(1)
		{
			// recv buf size
			remain_size = (start < end) ? (MAX_BUF_LEN - end) : (start - end);
			if(is_empty)
				remain_size = MAX_BUF_LEN - start;

			if(remain_size == 0)
				break;
			
			ret = recv(worker->conns->list[id].fd, buf + end, remain_size, 0); 

			if(ret > 0)
			{

				end = (end + ret) % (MAX_BUF_LEN);
				is_empty = false;
				read_len += ret;
				if(ret == remain_size)
				{
					continue;
				}
				else
				{
					break;
				}
				
			}

			// a stream socket peer perform an orderly shutdown, the return value will be 0;
			// the value 0 may alse be returned if the requested number of bytes to receive from a stream socket was 0
			if(ret == 0)
			{
			//	printf("connection closed by peer!!! buf: %p, %s, start: %d, end: %d, read_len: %d, errno: %d, %s\n", &buf[0], buf + 24, start, end, read_len, errno, strerror(errno));
				peer_close = true;
				break;
			}
			
			if(ret == -1)
			{
				if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
				{
					usleep(1000);
					continue;
				}

				peer_close = true;
				break;
			}

			//slog_write(LL_WARNING, "tcp recv loop, read_len: %d, start: %d, end: %d", read_len, start, end);
		}


		// need timeout control
		//if(read_len == 0 && is_empty)
		//	break;

		// data_len = read_len ??   get available data_len
		if( start < end )
		{
			data_len = end - start;
		}
		else
		{
			if(is_empty)
				data_len = 0;
			else
				data_len = MAX_BUF_LEN - start + end;
		}
		
		while(1)
		{
			if( 0 == data_len )
			{
				is_empty = true;
			}
			
			// no enough data buffer for LogHead
			if(data_len < sizeof(LogHead))
			{
				//slog_write(LL_WARNING, "no enough space for LogHead, size: %d", data_len);
				if(peer_close)
				{
					close_conn(worker, &(worker->conns->list[id]));
					//PUT_CONN_ITEM(worker->conns, &(worker->conns->list[id]));
					return ;
				}
				else 
					break ;
			
			}

			LogHead *head;
			if(MAX_BUF_LEN - start >= sizeof(LogHead))
			{
				head = (LogHead*)(buf + start);
			}
			else
			{
				int left = MAX_BUF_LEN - start;
				memcpy(head_tmp, buf + start, left);
				memcpy(head_tmp + left, buf, sizeof(LogHead) - left);
				head_tmp[sizeof(LogHead)] = 0;
				head = (LogHead*)head_tmp;
			}
			
			// ??????????
			if(head -> size <= 0 || head -> size > MAX_LOG_LEN)
			{
				slog_write(LL_WARNING, "unkown protocol!!!!");
				start = end;
				is_empty = true;
				close_conn(worker, &(worker->conns->list[id]));
				//PUT_CONN_ITEM(worker->conns, &(worker->conns->list[id]));

				return;
			}
			
			if(data_len < (head -> size + sizeof(LogHead)))
			{
				// continue read or close conn
				if(peer_close)
				{
					close_conn(worker, &(worker->conns->list[id]));
					//PUT_CONN_ITEM(worker->conns, &(worker->conns->list[id]));
					return ;
				}
				else 
					break;
			}

			// parse data 
			if((MAX_BUF_LEN - start) >= (sizeof(LogHead) + head->size))
			{
				queue_set_size = head->size + sizeof(LogHead);
				queue_set_data = buf + start;
			}
			else
			{

				int left = MAX_BUF_LEN - start;
				if(left > 0)
				{
					memcpy(log_tmp, buf + (start) % (MAX_BUF_LEN), left);
					memcpy(log_tmp + left, buf, head->size + sizeof(LogHead) - left);
				}
				else
				{
					memcpy(log_tmp, buf + (start + sizeof(LogHead))%(MAX_BUF_LEN), head->size);
				}

				//   -----------------------------
				//   | LogHead |     log body    |
				//   -----------------------------
				log_tmp[head->size + sizeof(LogHead)] = 0;

				// write to squeue
				queue_set_size = head->size + sizeof(LogHead);
				queue_set_data = log_tmp;
				
				//slog_write(LL_DEBUG, "SET DATA: %s\t SIZE: %d", queue_set_data, queue_set_size);
				
			}

			// write to squeue
			type_len = strlen(head->type);
			CRC_OFF(head->type, type_len, h);
			queue_writer = (QUEUE_WORKER*)&queue_threads[ h % conf.queue_worker_num ];

#ifdef DEBUG_TIME
	char timebuf[32];
	struct timespec timer;
	clock_gettime(CLOCK_REALTIME, &timer);
#endif

			//slog_write(LL_DEBUG, "SET DATA: %s\t TYPE: %s,SIZE: %d\t DATA_LEN: %d, START: %d, END: %d", queue_set_data + sizeof(LogHead), head -> type, head -> size, data_len, start, end);

			if(queue_set(queue_writer->queue, queue_set_data, queue_set_size))
			{
				slog_write(LL_WARNING, "SQUEUE SET FAILED");

				//continue;
			}
			else
			{
				// notify queue_thread
				write(queue_writer->notify_wfd, "", 1);
			}
			

#ifdef DEBUG_TIME
	struct timespec timer0;
	long long secdiff = 0, nsecdiff = 0;
	clock_gettime(CLOCK_REALTIME, &timer0);
	secdiff = diff(timer, timer0).tv_sec; 
	nsecdiff = diff(timer, timer0).tv_nsec; 
	long long total_diff = secdiff*1000 + nsecdiff/1000000; 
	slog_write(LL_DEBUG, "thread_id: %d, queue_set: %lld ms, queue_size: %d", worker -> thread_id, total_diff, queue_size(queue_writer->queue));
#endif 
			start = (start + sizeof(LogHead) + head->size) % (MAX_BUF_LEN);
			data_len -= (sizeof(LogHead) + head->size);
		}
				
	}
	
}


static void worker_receive_process0(int fd, int which, void *args){

	WORKER *worker = (WORKER*)args;
	uint32 id;
	if(read(fd, &id, 4) != 4)
	{
		sprintf(stderr, "worker receive process read pipe failed, errno: %d %m\n", errno);
		return;
	}
	printf("receive process, id: %d .....................\n", id);
	
	// worker recv setting
	struct timeval tv = {0};
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv));
	
//	setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char*)&tv, sizeof(tv));
//	int nRecvBuf = 32*1024;
//	setsockopt(worker->conns->list[id].fd, SOL_SOCKET, SO_RCVBUF, (const char*)&nRecvBuf, sizeof(int));

//	CONN *item;
//	GET_CONN_ITEM(worker->conns, item);
//	if(item == NULL)
//		continue;

	int start = 0, end = 0, read_len, write_len = 0;
	int ret, data_len;
	char head_tmp[25];
	char log_tmp[MAX_LOG_LEN + sizeof(LogHead)];
	bool is_empty = true;
	size_t remain_size;
	
	// for hash and squeue
	unsigned int type_len, h;
	unsigned int queue_set_size;
	char *queue_set_data;
	QUEUE_WORKER *queue_writer;

	char *buf = worker->conns->list[id].buf;
	while(1)
	{
		
		remain_size = MAX_BUF_LEN;
		read_len = 0;
		//recv_len = 0;
		
		while(1)
		{
			
			// remain_size = (start < end) ? (MAX_BUF_LEN - end) : (start - end);
			// if(is_empty)
			// 		remain_size = MAX_BUF_LEN;
			//

			if(start < end && end < MAX_BUF_LEN)
			{
				remain_size = MAX_BUF_LEN - end;
			}
			else if( (!is_empty) && 
					(end <= start) )
			{
				remain_size = start - end;
			}

			if(remain_size == 0)
				break;
			
			ret = recv(worker->conns->list[id].fd, buf + end % (MAX_BUF_LEN), remain_size, 0); 
		//	printf("tcp recv: %d, %s\n", ret, buf + end % (MAX_BUF_LEN));
			if(ret <= 0)
			{
				// printf("tcp recv, no data!!! buf: %p, start: %d, end: %d, read_len: %d, errno: %d, %s\n", &buf[0], start, end, read_len, errno, strerror(errno));
				break;
			}
			else
			{
				//printf("tcp recv: %d, %s\n", ret, buf + end % (MAX_BUF_LEN));
				is_empty = false;
				write_len = 0;
				read_len += ret;
			}

			end = (end + ret) % (MAX_BUF_LEN);
		}

		if(read_len == 0 && is_empty)
			break;

		while(1)
		{
			if(write_len == MAX_BUF_LEN)
			{
				is_empty = true;
			}

			if( start < end && end < MAX_BUF_LEN)
			{
				data_len = end - start;
			}
			else
			{
				if(is_empty)
				{
					data_len = 0;
				}
				else 
				{
					data_len = MAX_BUF_LEN - start + end;
				}
			}
			
			if(data_len < sizeof(LogHead))
			{
				//printf("can not get LogHead, size: %d, start: %d, end: %d, is_empty: %d\n", remain_size, start, end, is_empty);
				break;
				//if(is_empty && (remain_size == 0))
				//	break;             // ????????
				//else
				//	return;
			}

			LogHead *head;
			if(MAX_BUF_LEN - start >= sizeof(LogHead))
			{
				head = (LogHead*)(buf + start);
			}
			else
			{
				int left = MAX_BUF_LEN - start;
				memcpy(head_tmp, buf + start, left);
				memcpy(head_tmp + left, buf, sizeof(LogHead) - left);
				head_tmp[sizeof(LogHead)] = 0;
				head = (LogHead*)head_tmp;
			}
			
			// ??????????
			if(head -> size < 0 || head -> size > MAX_LOG_LEN)
			{
				printf("unkown protocol!!!!!\n");
				start = end;
				is_empty = true;
				
				close_conn(worker, &(worker->conns->list[id]));
				return;
			}
			
			if(data_len < (head->size + sizeof(LogHead)))
			{
			//	printf("continue read.......................\n");
				break;
			}

			// parse data 
			if((MAX_BUF_LEN - start) >= (sizeof(LogHead) + head->size))
			{
			//	printf("recv data: %s, size: %d, type: %s\n", buf + (start + sizeof(LogHead)), head->size, head->type);

				// write to squeue
				queue_set_size = head->size + sizeof(LogHead);
				queue_set_data = buf + start;

			}
			else
			{

				int left = MAX_BUF_LEN - start;
				//printf("pack~~~~~~~~~~~~~~ left: %d, start: %d\n", left, start);
				if(left > 0)
				{
					memcpy(log_tmp, buf + (start) % (MAX_BUF_LEN), left);
					memcpy(log_tmp + left, buf, head->size + sizeof(LogHead) - left);
				}
				else
				{
					memcpy(log_tmp, buf + (start + sizeof(LogHead))%(MAX_BUF_LEN), head->size);
				}

				log_tmp[head->size] = 0;
				//printf("(pack)recv data: %s, size: %d, type: %s\n", log_tmp + sizeof(LogHead), head->size, head->type);

				// write to squeue
				queue_set_size = head->size + sizeof(LogHead);
				queue_set_data = log_tmp;
			}

			// write to squeue
			type_len = strlen(head->type);
			CRC_OFF(head->type, type_len, h);
			queue_writer = (QUEUE_WORKER*)&queue_threads[ h % 16 ];

			//printf("queue thread: %d, id: %d, addr: %p, data: %s, size: %d\n", h%16, queue_writer->thread_id, queue_threads, queue_set_data + sizeof(LogHead), queue_set_size);

			if(queue_set(queue_writer->queue, queue_set_data, queue_set_size))
			{
				printf("squeue set error!!!!!!\n");
				//continue;
			}
			else
			{
				// notify queue_thread
				//printf("query write notify..............set_size: %d\n", queue_set_size);
				write(queue_writer->notify_wfd, "", 1);
			}

			

			start = (start + sizeof(LogHead) + head->size) % (MAX_BUF_LEN);
			write_len += sizeof(LogHead) + head->size;
		//	printf("###########read_len: %d############\n", read_len);
		}
				
	}
	
}

int accept_handler(int fd, struct sockaddr_in *s_in){
	int new_fd = -1;
	do {
		socklen_t len = sizeof(struct sockaddr_in);
		new_fd = accept(fd, (struct sockaddr_in*)&s_in, &len);
		
		if(new_fd < 0){
			if(errno == EINTR)
				continue;

			sprintf(stderr, "cannot accept, errno: %d %m\n", errno);
			break;
		}

		int flag = fcntl(new_fd, F_GETFL, 0);

		if(fcntl(new_fd, F_SETFL, flag|O_NONBLOCK) < 0){
			sprintf(stderr, "accept error, cannot set noblock IO, errno: %d %m\n", errno);
			safe_close(new_fd);
			return -1;
		}
		
		//printf("accept: %d\n", new_fd);
		return new_fd;
	}while(-1);

	return -1;
}

void server_accept_process(int fd, short which,  void *args){
	SERVER *server = args;
	struct sockaddr_in server_in;
	socklen_t len = sizeof(server_in);
	int acc_fd = accept(fd, (struct sockaddr*)&server_in, &len);
	//int acc_fd = accept_handler(fd, &server_in); 
	
	if(acc_fd == -1){
		// error log
		return;
	}

	printf("server accept: %d\n", acc_fd);
	int retry = 0;
//*	do{
//*		WORKER *worker = &server->workers->array[server->conn_count ++];
//*
//*		CONN *item;
//*		GET_CONN_ITEM(worker->conns, item);
//*		if(item == NULL)
//*			continue;
//*
//*		item -> fd = acc_fd;
//*		item -> ip = *(uint32*) &server_in.sin_addr;
//*		item -> port = (uint16) server_in.sin_port;
//*		write(worker->notified_wfd, (char*)&item->id, 4);
//*		//printf("write notified................\n");
//*
//*		return ;
//*	}while(++retry < server->workers->size/2);

	// err log
	safe_close(acc_fd);

}


int start_write_workers(WORKER_ARRAY *workers)
{
	return 0;
}

static int open_socket(const char *ip, short port, int backlog){
	printf("open server socket..................\n");
	int fd = -1;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd < 0){
		sprintf(stderr, "open server socket failed, errno: %d %m\n", errno);
		return -1;
	}

	evutil_make_socket_nonblocking(fd);
	
//	//unsigned long no_blocking = 1;
//	//ioctl(fd, FIOBIO, &no_blocking);
//	int flags = fcntl(fd, F_GETFL, 0);
//
//	if( fcntl(fd, F_SETFL, flags|O_NONBLOCK) < 0 ){
//		sprintf(stderr, "failed to set nonblocking IO, errno: %d %m\n", errno);
//		safe_close(fd);
//		return -1;
//	}

	struct timeval tv = {0};
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv));
	setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char*)&tv, sizeof(tv));

//	int flag_reuseaddr = 1;
//	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&flag_reuseaddr, sizeof(flag_reuseaddr));


	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);
	memset(&addr, 0, addrlen);

	if(ip == NULL)
		printf("ip: %s, port: %d\n", ip, port);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = (ip == NULL) ? INADDR_ANY : inet_addr(ip);

	if(bind(fd, (const struct sockaddr*)&addr, sizeof(addr)) < 0){
		sprintf(stderr, "failed to bind socket on port: %d, errno: %d %m\n", port, errno);
		safe_close(fd);
		return -1;  
	}

	if(listen(fd, backlog) < 0)
	{
		printf("can not listen on port: %d, errno: %d, %s\n", port, errno, strerror(errno));
		safe_close(fd);
		return -1;
	}


	return fd;
}


static void queue_handler_process(int fd, int which, void *args)
{
	QUEUE_WORKER *w = args;
	char buf[1];
	LogHead *head;
	size_t len = MAX_LOG_LEN + sizeof(LogHead);
	int ret;

	if(read(fd, buf, 1) != 1)
	{
		slog_write(LL_WARNING, "queue handler thread read pipe failed");
		return;
	}


	ret = queue_get(w->queue, w->data, &len);
	
	if(ret)
	{
		return;
	}


	head = (LogHead*)w->data;
	LogInfo *info;

	unsigned int type_len, h;
	
	type_len = strlen(head->type);
	HASH(head->type, type_len, h);
	HASHS_SEEK(table, h, info);

	if(info == NULL)
	{
		HASHS_INSERT(table, h, info);
		strcpy(info->type, head->type);
	}
	
	if(info->queue_info)
	{
		// send to MQ
		if((ret = send_queue_log(w, head, info)) != 0)
		{

			slog_write(LL_WARNING, "MQ SET FAILED, ret: %d", ret);
			return;
		}

	}

}


void *start_worker_routine(void *args)
{
	WORKER *w = args;
	//slog_write(LL_DEBUG, "start worker routine, tid: %d", w -> thread_id);
	event_base_loop(w->base, 0);

	return NULL;
}

void *start_queueworker_routine(void *args)
{
	QUEUE_WORKER *w = args;
	//slog_write(LL_DEBUG, "start queueworker routine, tid: %d", w -> thread_id);
	event_base_loop(w->base, 0);

	return NULL;
}


static int setup_queue_workers(uint16 workernum, int read_timeout, int write_timeout)
{
	//slog_write(LL_DEBUG, "init  queue workers");
	uint32 len = sizeof(QUEUE_WORKER) * workernum;
	queue_threads = (QUEUE_WORKER*)malloc(len);

	if(queue_threads == NULL){
		//slog_write(LL_DEBUG, "init  queue workers failed, errno: %d", errno);
		return -1;
	}

	memset(queue_threads, 0, len);
	uint16 i;
	for( i = 0; i < workernum; i++)
	{
		int fds[2];
		if(pipe(fds)){
			slog_write(LL_WARNING, "init queue workers create pipe error: %d", errno);
			return -1;
		}

		queue_threads[i].id = i;
		queue_threads[i].queue = queue_init(MAX_BUFFER_SIZE);
		queue_threads[i].notify_rfd = fds[0];
		queue_threads[i].notify_wfd = fds[1];
		
		queue_threads[i].base = event_init();
		if(queue_threads[i].base == NULL)
		{
			slog_write(LL_WARNING, "init queue workers cannot allocate event base");
			return -1;
		}

		queue_threads[i].queue_event = event_new(queue_threads[i].base, queue_threads[i].notify_rfd, EV_READ | EV_PERSIST, queue_handler_process, (void*)&queue_threads[i]);

		if(event_add(queue_threads[i].queue_event, 0) == -1){
			slog_write(LL_WARNING, "queue_event event_add failed!!!!!!!!!");
			return -1;
		}

	}


	// start queue worker
	for( i = 0; i < workernum; i++)
	{
		if(pthread_create(&queue_threads[i].thread_id, NULL, start_queueworker_routine, (void*)&queue_threads[i]) != 0)
		{
			slog_write(LL_WARNING, "init queue worker pthread create failed!!!!!!!!!");
			return -1;
		}
	}

	return 0;
   
}


	
static int setup_workers(uint16 workernum, uint32 connnum, int read_timeout, int write_timeout){
	//slog_write(LL_DEBUG, "init workers");
	uint32 len = sizeof(WORKER) * workernum;
	worker_threads = (WORKER*)malloc(len);
	if(worker_threads == NULL){
		slog_write(LL_WARNING, "init workers malloc error");
		return -1;
	}

	memset(worker_threads, 0, len);
	
	uint16 i;
	for(i = 0; i < workernum; i++){
		int fds[2];
		if(pipe(fds)){
			slog_write(LL_WARNING, "init queue workers create pipe error: %d", errno);
			return -1;
		}
	
		
		worker_threads[i].id = i;
		worker_threads[i].notified_rfd = fds[0];
		worker_threads[i].notified_wfd = fds[1];

		worker_threads[i].base = event_init();
		if(worker_threads[i].base == NULL){
			exit(1);
		}

		worker_threads[i].read_event = event_new(worker_threads[i].base, worker_threads[i].notified_rfd, EV_READ | EV_PERSIST, worker_receive_process, (void*)&worker_threads[i]);
//		event_set(&w->read_event, w->notified_rfd, EV_READ | EV_PERSIST, worker_receive_process, w);
//		event_base_set(w->base, &w->read_event);

		// write operation
		//event_set(&w->write_event,  

		if(event_add(worker_threads[i].read_event, 0) == -1){
			slog_write(LL_WARNING, "init workers event_add failed!!!!!!!!!");
			return -1;
		}
		
		CONN_LIST *list = init_conn_list(connnum);
		worker_threads[i].conns = list;

	}

	for( i = 0; i < workernum; i++)
	{
		if(pthread_create(&worker_threads[i].thread_id, NULL, start_worker_routine, (void*)&worker_threads[i]) != 0){
			slog_write(LL_WARNING, "init workers pthread create failed!!!!!!!!!");
			return -1;
		}
	}

	return 0;


//	return workers;

}


static int idx = -1;

void do_accept(int fd, short event, void* args){
	//slog_write(LL_DEBUG, "do accept~~~~~~~~");
	SERVER *s = args;
	struct sockaddr addr;
	socklen_t len = sizeof(addr);
	int new_fd = accept(fd, (struct sockaddr*)&addr, &len);
	int nRecvBuf = 64*1024;

	if(_set_nonblocking(new_fd) == -1){
		slog_write(LL_WARNING, "listen socket nonblocking set failed!!!!!!");
		close(new_fd);
		return;
	}

//	setsockopt(new_fd, SOL_SOCKET, SO_RCVBUF, (const char*)&nRecvBuf, sizeof(int));
	
//	struct timeval tv = {0};
//	tv.tv_sec = 3;
//	tv.tv_usec = 0;
//	setsockopt(new_fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv));

	// set keepalive
//	bool keepalive = TRUE;
//	setsockopt(new_fd, SOL_SOCKET, SO_KEEEPALIVE, (char *)&keepalive, sizeof(keepalive));

//FILE *fp;
//fp = fopen("tcpdata", "a+");


	if(new_fd < 0){
		slog_write(LL_WARNING, "accept failed!!!!!!");
	}
	else if( fd > FD_SETSIZE){
		slog_write(LL_WARNING, "connects exceed FD_SIZE !!!!!!");
		close(new_fd);
	}
	else{
//		printf("accept\n");
		//	int idx = 0;
		int retry = 0;
		do {
			idx = (idx+1) % (conf.worker_num);
			WORKER w = worker_threads[idx];
			CONN* item;
			GET_CONN_ITEM(w.conns, item);
			if(item == NULL){
				slog_write(LL_WARNING, "item is NULL!!!");
				continue;
			}

			item -> fd = new_fd;
			inet_ntop(AF_INET, &((struct sockaddr_in *)&addr) -> sin_addr, item -> ip, 15);
			item -> ip[16] = 0;
			//item -> ip = inet_ntop(AF_INET, &addr, item -> ip, 16);
			//snprintf(item -> ip, 16, "%s", inet_ntoa(addr.sin_addr));
			item -> port = ntohs(((struct sockaddr_in *)&addr) -> sin_port);
			write(w.notified_wfd, (char*)&item->id, 4);
			return;
		} while(retry++ < 10);
		
		slog_write(LL_WARNING, "no free conns!!!");
		safe_close(new_fd);
	}

//fclose(fp);
}


struct timeval tv;
void cb_timer(int fd, short event, void *args)
{
	evtimer_add(*((struct event**)args), &tv);
}


//捕获core信息，打印调用堆栈
static void sig_segv(int sig){
	char tmp[256];
	sprintf(tmp, "mycore.%d", getpid());
//	unlink(tmp);
	int fd = open(tmp, O_RDWR | O_CREAT);
	if(fd > 0){
		int ret = sprintf(tmp, "recv signal:%d\n", sig);
		write(fd, tmp, ret);

		void *stacks[100];
		char **strings;
		int num = backtrace(stacks, 100);
		strings = backtrace_symbols(stacks, num);
		int i;
		for(i = 0; i < num; i ++){
			int ret = sprintf(tmp, "%p\t%s\n", stacks[i], strings[i]);
			write(fd, tmp, ret);
		}
		free(strings);
	}
	int i; //关闭文件句柄，是为了停止socket服务
	for(i = 0; i < 1024; i ++){
		close(i);
	}
	signal(SIGKILL, SIG_DFL);
	signal(SIGSEGV, SIG_DFL);
	signal(SIGABRT, SIG_DFL);
}


int main(int argc, char **argv){
//int main(){
	

	char *conf_file = "log_test.conf";
	if(argc > 1)
	{
		conf_file = argv[1];
	}
	
	if(run_daemon(1, 1) == -1)
	{
		printf("run daemon failed !!!!!!!\n");
		exit(1);
	}

	signal(SIGSEGV, sig_segv);
	signal(SIGABRT, sig_segv);
	signal(SIGKILL, sig_segv);

	conf.argc = argc;
	conf.argv = argv;
	
	
	HASHS_INIT(table, MAX_TYPE_NUM);

	if(load_conf(&conf, conf_file, &table) < 0)
	{
		printf("load config failed!!!!!\n");
		exit(1);
	}

	int addr_len = 0;
	struct sockaddr_in addr;
	//int port = 50000;
	//char *ip = "10.73.12.142";
	addr.sin_family = AF_INET;
	addr.sin_port = htons(conf.port);
	addr.sin_addr.s_addr = conf.ip == NULL ? INADDR_ANY: inet_addr(conf.ip);
	//addr.sin_port = htons(conf->port);
	//addr.sin_addr.s_addr = inet_addr(conf->ip);
	addr_len = sizeof(addr);

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	//evutil_make_socket_nonblocking(fd);
	int ret = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&ret, 4);
	_set_nonblocking(fd);

	if(bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0){
		perror("bind failed\n");
		close(fd);
		exit(1);
	}
	
	if(listen(fd, conf.backdog) < 0){
		perror("listen failed\n");
		close(fd);
		exit(1);
	}

	// ===========================
	//			init hashtable
	// ===========================
	
//	HASHS_INIT(table, MAX_TYPE_NUM);

//	//init_table();
//
//	char *queue_name = "data1@10.0.0.1:11993";
//	char *type_name = "type_pages";
//	int len = strlen(type_name);
//
//	unsigned int h;
//	LogInfo *node;
//	HASH(type_name, len, h);
//	HASHS_INSERT(table, h, node);
//	if(node){
//		strcpy(node->type, type_name);
//		strcpy(node->queue_info, queue_name);
//	}

	// ===========================
	//		  queue_worker thread
	// ===========================
	
	if(setup_queue_workers(conf.queue_worker_num, 5, 5) == -1)
	{
		printf("setup queue workers failed !!!!!\n");
		exit(1);
	}
	printf("setup queue workers ok \n");

	//============================
	//		worker
	//============================		

	if(setup_workers(conf.worker_num, conf.conn_num, 5, 5) == -1)
	{
		printf("setup workers failed !!!!!\n");
		exit(1);
	}
	printf("setup workers ok\n");

	//============================
	//		server
	//============================		
	server = (SERVER*)malloc(sizeof(SERVER));

	//struct event_base *base = event_base_new();
	evutil_socket_t listener;
	struct event *listener_event;
	server->base = event_init();
	//struct event_base *base = event_init();
	listener_event = event_new(server->base, fd, EV_READ | EV_PERSIST, do_accept, (void*)server);
	//event_set(&listener_event, fd, EV_READ | EV_PERSIST, do_accept, 
	
	// timer
//	struct event *timer_ev = NULL;
//	timer_ev = evtimer_new(server->base, cb_timer, (void*)&timer_ev);
//	if(!timer_ev)
//	{
//		return -1;
//	}
//	memset(&tv, 0, sizeof(tv));
//	tv.tv_sec = 0;
//	tv.tv_usec = 100000;
//	evtimer_add(timer_ev, &tv);

	event_add(listener_event, NULL);

	event_base_loop(server->base, 0);

	close(fd);

	return 0;
}
