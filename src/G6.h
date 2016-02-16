/*
 * G6 - TCP Transfer && LB Dispenser
 * Author      : calvin
 * Email       : calvinwillliams@163.com
 *
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

#ifndef _H_G6_
#define _H_G6_

#if ( defined __linux ) || ( defined __unix )
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <stdarg.h>
#include <limits.h>
#include <signal.h>
#include <sys/wait.h>
#include <pthread.h>
#define _VSNPRINTF		vsnprintf
#define _SNPRINTF		snprintf
#define _CLOSESOCKET		close
#define _ERRNO			errno
#define _EWOULDBLOCK		EWOULDBLOCK
#define _ECONNABORTED		ECONNABORTED
#define _EINPROGRESS		EINPROGRESS
#define _ECONNRESET		ECONNRESET
#define _ENOTCONN		ENOTCONN
#define _EISCONN		EISCONN
#define _SOCKLEN_T		socklen_t
#define _GETTIMEOFDAY(_tv_)	gettimeofday(&(_tv_),NULL)
#define _LOCALTIME(_tt_,_stime_) \
	localtime_r(&(_tt_),&(_stime_));
#elif ( defined _WIN32 )
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <io.h>
#include <windows.h>
#define _VSNPRINTF		_vsnprintf
#define _SNPRINTF		_snprintf
#define _CLOSESOCKET		closesocket
#define _ERRNO			GetLastError()
#define _EWOULDBLOCK		WSAEWOULDBLOCK
#define _ECONNABORTED		WSAECONNABORTED
#define _EINPROGRESS		WSAEINPROGRESS
#define _ECONNRESET		WSAECONNRESET
#define _ENOTCONN		WSAENOTCONN
#define _EISCONN		WSAEISCONN
#define _SOCKLEN_T		int
#define _GETTIMEOFDAY(_tv_) \
	{ \
		SYSTEMTIME stNow ; \
		GetLocalTime( & stNow ); \
		(_tv_).tv_usec = stNow.wMilliseconds * 1000 ; \
		time( & ((_tv_).tv_sec) ); \
	}
#define _SYSTEMTIME2TIMEVAL_USEC(_syst_,_tv_) \
		(_tv_).tv_usec = (_syst_).wMilliseconds * 1000 ;
#define _SYSTEMTIME2TM(_syst_,_stime_) \
		(_stime_).tm_year = (_syst_).wYear - 1900 ; \
		(_stime_).tm_mon = (_syst_).wMonth - 1 ; \
		(_stime_).tm_mday = (_syst_).wDay ; \
		(_stime_).tm_hour = (_syst_).wHour ; \
		(_stime_).tm_min = (_syst_).wMinute ; \
		(_stime_).tm_sec = (_syst_).wSecond ;
#define _LOCALTIME(_tt_,_stime_) \
	{ \
		SYSTEMTIME	stNow ; \
		GetLocalTime( & stNow ); \
		_SYSTEMTIME2TM( stNow , (_stime_) ); \
	}
#endif

#include "LOGC.h"

#ifndef ULONG_MAX
#define ULONG_MAX 0xffffffffUL
#endif

#define FOUND				9	/* 找到 */
#define NOT_FOUND			4	/* 找不到 */

#define MATCH				1	/* 匹配 */
#define NOT_MATCH			-1	/* 不匹配 */

#define LOAD_BALANCE_ALGORITHM_G	"G"	/* 管理端口 */
#define LOAD_BALANCE_ALGORITHM_MS	"MS"	/* 主备模式 */
#define LOAD_BALANCE_ALGORITHM_RR	"RR"	/* 轮询模式 */
#define LOAD_BALANCE_ALGORITHM_LC	"LC"	/* 最少连接模式 */
#define LOAD_BALANCE_ALGORITHM_RT	"RT"	/* 最小响应时间模式 */
#define LOAD_BALANCE_ALGORITHM_RD	"RD"	/* 随机模式 */
#define LOAD_BALANCE_ALGORITHM_HS	"HS"	/* HASH模式 */

#define DEFAULT_RULES_INITCOUNT			2
#define DEFAULT_RULES_INCREASE			5

#define DEFAULT_CLIENTS_INITCOUNT_IN_ONE_RULE	2
#define DEFAULT_CLIENTS_INCREASE_IN_ONE_RULE	5
#define DEFAULT_FORWARDS_INITCOUNT_IN_ONE_RULE	2
#define DEFAULT_FORWARDS_INCREASE_IN_ONE_RULE	5
#define DEFAULT_SERVERS_INITCOUNT_IN_ONE_RULE	2
#define DEFAULT_SERVERS_INCREASE_IN_ONE_RULE	5

#define DEFAULT_FORWARD_SESSIONS_MAXCOUNT	1024	/* 缺省最大连接数量 */
#define DEFAULT_FORWARD_TRANSFER_BUFSIZE	4096	/* 缺省通讯转发缓冲区大小 */

/* 网络地址信息结构 */
struct NetAddress
{
	char			ip[ 30 + 1 ] ; /* ip地址 */
	char			port[ 20 + 1 ] ; /* 端口 */
	struct sockaddr_in	sockaddr ; /* sock地址结构 */
} ;

/* 客户端信息结构 */
struct ClientNetAddress
{
	struct NetAddress	netaddr ; /* 网络地址结构 */
	int			sock ; /* sock描述字 */
	
	unsigned long		client_connection_count ; /* 客户端连接数量 */
	unsigned long		maxclients ; /* 最大客户端数量 */
} ;

/* 转发端信息结构 */
struct ForwardNetAddress
{
	struct NetAddress	netaddr ; /* 网络地址结构 */
	int			sock ; /* sock描述字 */
} ;

/* 服务端信息结构 */
struct ServerNetAddress
{
	struct NetAddress	netaddr ; /* 网络地址结构 */
	int			sock ; /* sock描述字 */
	
	unsigned long		server_connection_count ; /* 服务端连接数量 */
} ;

/* 转发规则结构 */
#define RULE_ID_MAXLEN			64
#define LOAD_BALANCE_ALGORITHM_MAXLEN	2

struct ForwardRule
{
	char				rule_id[ RULE_ID_MAXLEN + 1 ] ; /* 规则ID � */
	char				load_balance_algorithm[ LOAD_BALANCE_ALGORITHM_MAXLEN + 1 ] ; /* 负载均衡算法 */
	
	struct ClientNetAddress		*clients_addr ; /* 客户端地址结构 */
	unsigned long			clients_addr_size ; /* 客户端规则配置最大数量 */
	unsigned long			clients_addr_count ; /* 客户端规则配置数量 */
	
	struct ForwardNetAddress	*forwards_addr ; /* 转发端地址结构 */
	unsigned long			forwards_addr_size ; /* 转发端规则配置最大数量 */
	unsigned long			forwards_addr_count ; /* 转发端规则配置数量 */
	
	struct ServerNetAddress		*servers_addr ; /* 服务端地址结构 */
	unsigned long			servers_addr_size ; /* 服务端规则配置最大数量 */
	unsigned long			servers_addr_count ; /* 服务端规则配置数量 */
	unsigned long			selects_addr_index ; /* 当前服务端索引 */
	
	union
	{
		struct
		{
			unsigned long	server_unable ; /* 服务不可用暂禁次数 */
		} *RR ;
		struct
		{
			unsigned long	server_unable ; /* 服务不可用暂禁次数 */
		} *LC ;
		struct
		{
			unsigned long	server_unable ; /* 服务不可用暂禁次数 */
			struct timeval	tv1 ; /* 最近读时间戳 */
			struct timeval	tv2 ; /* 最近写时间戳 */
			struct timeval	dtv ; /* 最近读写时间戳差 */
		} *RT ;
	} status ;
	
} ;

#define FORWARD_SESSION_STATUS_CONNECTING	1 /* 非堵塞连接中 */
#define FORWARD_SESSION_STATUS_CONNECTED	2 /* 连接完成 */

#define IO_BUFFER_SIZE				4096 /* 输入输出缓冲区大小 */

struct IoBuffer
{
	char			buffer[ IO_BUFFER_SIZE + 1 ] ;
	unsigned long		buffer_len ;
} ;

/* 转发会话结构 */
struct ForwardSession
{
	struct ForwardRule	*p_forward_rule ;
	
	unsigned char		status ;
	
	struct IoBuffer		io_buffer ;
	
	struct ForwardSession	*p_reverse_forward_session ;
} ;

/* 命令行参数 */
struct CommandParameter
{
	char			*config_pathfilename ; /* -f ... */
	unsigned long		forward_thread_count ; /* -t ... */
	unsigned long		forward_session_maxcount ; /* -s ... */
} ;

/* 服务器环境结构 */
struct ServerEnv
{
	struct CommandParameter	cmd_para ;
	
	pid_t			pid ;
	int			pipefds[2] ;
	
	struct ForwardRule	*forward_rules ;
	unsigned long		forward_rules_size ;
	unsigned long		forward_rules_count ;
	
	struct ForwardSession	*forward_sessions ;
	unsigned long		forward_session_count ;
	unsigned long		forward_session_use_offsetpos ;
} ;

/********* util *********/

int Rand( int min, int max );
unsigned long CalcHash( char *str );
int SetReuseAddr( int sock );
int SetNonBlocking( int sock );
int BindDaemonServer( char *pcServerName , int (* ServerMain)( void *pv ) , void *pv , int (* ControlMain)(long lControlStatus) );

/********* LoadConfig *********/

int LoadConfig( struct ServerEnv *penv );
void UnloadConfig( struct ServerEnv *penv );

/********* MonitorProcess *********/

int _MonitorProcess( void *pv );

/********* WorkerProcess *********/

int WorkerProcess( struct ServerEnv *penv );

/********* AcceptThread *********/

void _AcceptThread( void *pv );

/********* ForwardThread *********/

void _ForwardThread( void *pv );

#endif

