/*
 * twemproxy - A fast and lightweight proxy for memcached protocol.
 * Copyright (C) 2011 Twitter, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _NC_STATS_H_
#define _NC_STATS_H_

#include <nc_core.h>

//���Բο�stats_pool_field���÷�    
#define STATS_POOL_CODEC(ACTION)                                                                                 \
    /* client behavior */                                                                                           \
    ACTION( client_eof,             STATS_COUNTER,      "# eof on client connections")                              \
    ACTION( client_err,             STATS_COUNTER,      "# errors on client connections")                           \
    ACTION( client_connections,     STATS_GAUGE,        "# active client connections")                              \
    /* pool behavior */                                                                                             \
    ACTION( server_ejects,          STATS_COUNTER,      "# times backend server was ejected")                       \
    /* forwarder behavior */                                                                                        \
    ACTION( forward_error,          STATS_COUNTER,      "# times we encountered a forwarding error")                \
    ACTION( fragments,              STATS_COUNTER,      "# fragments created from a multi-vector request")          \

//���Բο�stats_server_field���÷�   
#define STATS_SERVER_CODEC(ACTION)                                                                                  \
    /* server behavior */                                                                                           \
    ACTION( server_eof,             STATS_COUNTER,      "# eof on server connections")                              \
    ACTION( server_err,             STATS_COUNTER,      "# errors on server connections")                           \
    ACTION( server_timedout,        STATS_COUNTER,      "# timeouts on server connections")                         \
    ACTION( server_connections,     STATS_GAUGE,        "# active server connections")                              \
    ACTION( server_ejected_at,      STATS_TIMESTAMP,    "timestamp when server was ejected in usec since epoch")    \
    /* data behavior */                                                                                             \
    /*�ͻ���������  �ο� req_forward_stats //�ͻ��������ֽ���  �ο� req_forward_stats*/         \
    ACTION( requests,               STATS_COUNTER,      "# requests")                                               \
    ACTION( request_bytes,          STATS_COUNTER,      "total request bytes")                                      \
    ACTION( responses,              STATS_COUNTER,      "# responses")                                              \
    ACTION( response_bytes,         STATS_COUNTER,      "total response bytes")                                     \
    /* req_server_dequeue_imsgq */   \
    ACTION( in_queue,               STATS_GAUGE,        "# requests in incoming queue")                             \
    ACTION( in_queue_bytes,         STATS_GAUGE,        "current request bytes in incoming queue")                  \
    ACTION( out_queue,              STATS_GAUGE,        "# requests in outgoing queue")                             \
    ACTION( out_queue_bytes,        STATS_GAUGE,        "current request bytes in outgoing queue")                  \

/*
[root@s10-2-20-2 twemproxy]# telnet 127.0.0.1 22222
Trying 127.0.0.1...
Connected to 127.0.0.1.
Escape character is '^]'.
{
"service":"nutcracker", "source":"s10-2-20-2.hx", "version":"0.4.1", "uptime":25, "timestamp":1467272574, 
"total_connections":1, "curr_connections":1, 
"alpha": {
        "client_eof":0, "client_err":0, "client_connections":0, "server_ejects":0, "forward_error":0, "fragments":0, 
        "server1": {
            "server_eof":0, "server_err":0, "server_timedout":0, "server_connections":0, "server_ejected_at":0, 
            "requests":0, "request_bytes":0, "responses":0, "response_bytes":0, 
            "in_queue":0, "in_queue_bytes":0, "out_queue":0, "out_queue_bytes":0
        },
        "server2": {
            "server_eof":0, "server_err":0,  "server_timedout":0, "server_connections":0, "server_ejected_at":0, "requests":0, 
            "request_bytes":0, "responses":0, "response_bytes":0, "in_queue":0, "in_queue_bytes":0, "out_queue":0, "out_queue_bytes":0
        },
        "server3": {
            "server_eof":0, "server_err":0, "server_timedout":0, "server_connections":0, "server_ejected_at":0,
            "requests":0, "request_bytes":0, "responses":0, "response_bytes":0, "in_queue":0, "in_queue_bytes":0,
            "out_queue":0, "out_queue_bytes":0
        },
        "server4": {
            "server_eof":0, "server_err":0, "server_timedout":0, "server_connections":0, 
            "server_ejected_at":0, "requests":0, "request_bytes":0, "responses":0, "response_bytes":0, "in_queue":0, 
            "in_queue_bytes":0, "out_queue":0, "out_queue_bytes":0
        }
    }
}
Connection closed by foreign host.
*/
#define STATS_ADDR      "0.0.0.0"
#define STATS_PORT      22222
#define STATS_INTERVAL  (30 * 1000) /* in msec */

typedef enum stats_type { //�ο�stats_metric_init
    STATS_INVALID,
    STATS_COUNTER,    /* monotonic accumulator */
    STATS_GAUGE,      /* non-monotonic accumulator */
    STATS_TIMESTAMP,  /* monotonic timestamp (in nsec) */
    STATS_SENTINEL
} stats_type_t;

//�ýṹ������stats_make_rsp�л�������͸��ͻ���

//�����ռ�͸�ֵ��stats_pool_metric_init
struct stats_metric { //stats_pool->metric�еĳ�Ա           ���Բο�stats_server_to_metric
    stats_type_t  type;         /* type */
    struct string name;         /* name (ref) */
    union {
        int64_t   counter;      /* accumulating counter */
        int64_t   timestamp;    /* monotonic timestamp */
    } value;
};

//�����ռ�͸�ֵ��stats_server_map->stats_server_init
struct stats_server {
    //server_pool->name
    struct string name;   /* server name (ref) */ //
    //�����ԱΪstats_metric
    struct array  metric; /* stats_metric[] for server codec */
};

//������server������alpha delta�ȸ��Զ�Ӧһ��stats_poll�ṹ������stats->current,stats->shadow,stats->sum�У�
//�����ռ�͸�ֵ��stats_pool_map
struct stats_pool {
    //Ҳ����server_pool->name�еĴ�server��������alpha
    struct string name;   /* pool name (ref) */ //��server����ֵ��stats_begin_nesting
    //�����ռ�͸�ֵ��stats_pool_init->stats_pool_metric_init����Ա����Ϊstats_metric ,���������������stats_pool_codec
    struct array  metric; /* stats_metric[] for pool codec */ //�������е�������Դ��stats_pool_codec
    //�����ռ�͸�ֵ��stats_pool_init->stats_server_map����Ա����Ϊstats_server
    //��server�е�server:�����б��ж��ٸ��� 
    struct array  server; /* stats_server[] */ //�������е���Դ��nutcracker.yul�����ļ��е�server�б���Ϣ
};

struct stats_buffer {
    size_t   len;   /* buffer length */
    uint8_t  *data; /* buffer data */
    size_t   size;  /* buffer alloc size */
};

/*
[root@s10-2-20-2 twemproxy]# telnet 127.0.0.1 22222
Trying 127.0.0.1...
Connected to 127.0.0.1.
Escape character is '^]'.
{
"service":"nutcracker", "source":"s10-2-20-2.hx", "version":"0.4.1", "uptime":25, "timestamp":1467272574, 
"total_connections":1, "curr_connections":1, 
"alpha": {
        "client_eof":0, "client_err":0, "client_connections":0, "server_ejects":0, "forward_error":0, "fragments":0, 
        "server1": {
            "server_eof":0, "server_err":0, "server_timedout":0, "server_connections":0, "server_ejected_at":0, 
            "requests":0, "request_bytes":0, "responses":0, "response_bytes":0, 
            "in_queue":0, "in_queue_bytes":0, "out_queue":0, "out_queue_bytes":0
        },
        "server2": {
            "server_eof":0, "server_err":0,  "server_timedout":0, "server_connections":0, "server_ejected_at":0, "requests":0, 
            "request_bytes":0, "responses":0, "response_bytes":0, "in_queue":0, "in_queue_bytes":0, "out_queue":0, "out_queue_bytes":0
        },
        "server3": {
            "server_eof":0, "server_err":0, "server_timedout":0, "server_connections":0, "server_ejected_at":0,
            "requests":0, "request_bytes":0, "responses":0, "response_bytes":0, "in_queue":0, "in_queue_bytes":0,
            "out_queue":0, "out_queue_bytes":0
        },
        "server4": {
            "server_eof":0, "server_err":0, "server_timedout":0, "server_connections":0, 
            "server_ejected_at":0, "requests":0, "request_bytes":0, "responses":0, "response_bytes":0, "in_queue":0, 
            "in_queue_bytes":0, "out_queue":0, "out_queue_bytes":0
        }
    }
}
Connection closed by foreign host.
\
*/

//�ýṹ�ռ������context->stats
//�����ռ�͸�ֵ��stats_create
struct stats {//�洢��context->stats��Ա�У���stats_create
    //�����˿�  //������ַ�Ͷ˿� ����ͨ��-s���ã���ֵ��stats_create
    uint16_t            port;            /* stats monitoring port */
    int                 interval;        /* stats aggregation interval */ //-i��������
    struct string       addr;            /* stats monitoring address */

    //���һ�οͻ��˻�ȡͳ����Ϣ��ʱ�����Ŀ�ļ���ͻ�������ͳ��֮�����˶���
    int64_t             start_ts;        /* start timestamp of nutcracker */
    //�����ռ�͸�ֵ��stats_create_buf Ӧ���telnet xxx.x.x.x 22222�ı������ݶ�����stats->buf�е�
    struct stats_buffer buf;             /* output buffer */

    //������server������alpha delta�ȸ��Զ�Ӧһ��stats_poll�ṹ������stats->current,stats->shadow,stats->sum�У�

    /* ���е�ͳ�ƺͶ�����뵽sum�У�һ��ʱ���ڵ�ͳ�Ʒ���current��,curren�е�ͳ��ʵ�������ϴλ�ȡ��Ϣ��
    ��ǰ��λ�ȡ��Ϣ���ʱ����ڵ�ͳ����Ϣ,��λ�ȡ���ͻ�������22222stat�˿ڵ�ʱ��stats_swap���curren�е����ݿ�����
    shadow�У�stats_aggregateȻ���shadow��sum��ͷ���sum�����Ӧ����ͻ��� */
    //��Ա����stats_pool����stats_create->stats_pool_map
    //���������������Ա���Ͷ���stats_pool����stats_create->stats_pool_map
    //current shadow sum��������ϵ���Բο�stats_swap stats_aggregate
    struct array        current;         /* stats_pool[] (a) */
    struct array        shadow;          /* stats_pool[] (b) */
    //sum�Ƕ�shadow�е������Ϣ������ͣ���stats_aggregate  
    /* ���е�ͳ�ƺͶ�����뵽sum�У�һ��ʱ���ڵ�ͳ�Ʒ���current��,curren�е�ͳ��ʵ�������ϴλ�ȡ��Ϣ��
    ��ǰ��λ�ȡ��Ϣ���ʱ����ڵ�ͳ����Ϣ,��λ�ȡ���ͻ�������22222stat�˿ڵ�ʱ��stats_swap���curren�е����ݿ�����
    shadow�У�stats_aggregateȻ���shadow��sum��ͷ���sum�����Ӧ����ͻ��� */
    struct array        sum;             /* stats_pool[] (c = a + b) */

    pthread_t           tid;             /* stats aggregator thread */
    //�׽��ּ�stats_listen  epoll�����¼���event_loop_stats  ���ܿͻ������Ӽ�����stats��Ӧ��stats_send_rsp
    int                 sd;              /* stats descriptor */

    //stat��֯��ʽ���Բο�stats_create_buf
    struct string       service_str;     /* service string */
    struct string       service;         /* service */
    struct string       source_str;      /* source string */
    struct string       source;          /* source */
    struct string       version_str;     /* version string */
    struct string       version;         /* version */
    struct string       uptime_str;      /* uptime string */
    struct string       timestamp_str;   /* timestamp string */
    struct string       ntotal_conn_str; /* total connections string */
    struct string       ncurr_conn_str;  /* curr connections string */

    //stats_swap����1  ֻ�пͻ��˷�����������ȡstats��Ϣ��ʱ����stats_aggregateͳ�������0��
    volatile int        aggregate;       /* shadow (b) aggregate? */
    //stats_server_to_metric����1  ֻҪ��ͳ����Ϣ�����˸�������ͻ���1
    volatile int        updated;         /* current (a) updated? */
};

#define DEFINE_ACTION(_name, _type, _desc) STATS_POOL_##_name,
typedef enum stats_pool_field {
    /*
    ת�����enum��ԱΪ 
    enum stats_pool_field {
        STATS_POOL_client_eof,
        STATS_POOL_client_err,
        ......
        STATS_POOL_out_queue_bytes,
        STATS_POOL_NFIELD
    }
    */
    STATS_POOL_CODEC(DEFINE_ACTION) 
    STATS_POOL_NFIELD
} stats_pool_field_t;
#undef DEFINE_ACTION

#define DEFINE_ACTION(_name, _type, _desc) STATS_SERVER_##_name,
typedef enum stats_server_field {
    /*
    ת�����enum��ԱΪ 
    enum stats_pool_field {
        STATS_SERVER_server_eof,
        STATS_SERVER_server_err,
        ......
        STATS_SERVER_out_queue_bytes,
        STATS_SERVER_NFIELD
    }
    */
    STATS_SERVER_CODEC(DEFINE_ACTION)
    STATS_SERVER_NFIELD
} stats_server_field_t;
#undef DEFINE_ACTION

#if defined NC_STATS && NC_STATS == 1

#define stats_pool_incr(_ctx, _pool, _name) do {                        \
    _stats_pool_incr(_ctx, _pool, STATS_POOL_##_name);                  \
} while (0)

#define stats_pool_decr(_ctx, _pool, _name) do {                        \
    _stats_pool_decr(_ctx, _pool, STATS_POOL_##_name);                  \
} while (0)

#define stats_pool_incr_by(_ctx, _pool, _name, _val) do {               \
    _stats_pool_incr_by(_ctx, _pool, STATS_POOL_##_name, _val);         \
} while (0)

#define stats_pool_decr_by(_ctx, _pool, _name, _val) do {               \
    _stats_pool_decr_by(_ctx, _pool, STATS_POOL_##_name, _val);         \
} while (0)

#define stats_pool_set_ts(_ctx, _pool, _name, _val) do {                \
    _stats_pool_set_ts(_ctx, _pool, STATS_POOL_##_name, _val);          \
} while (0)

#define stats_server_incr(_ctx, _server, _name) do {                    \
    _stats_server_incr(_ctx, _server, STATS_SERVER_##_name);            \
} while (0)

#define stats_server_decr(_ctx, _server, _name) do {                    \
    _stats_server_decr(_ctx, _server, STATS_SERVER_##_name);            \
} while (0)

#define stats_server_incr_by(_ctx, _server, _name, _val) do {           \
    _stats_server_incr_by(_ctx, _server, STATS_SERVER_##_name, _val);   \
} while (0)

#define stats_server_decr_by(_ctx, _server, _name, _val) do {           \
    _stats_server_decr_by(_ctx, _server, STATS_SERVER_##_name, _val);   \
} while (0)

#define stats_server_set_ts(_ctx, _server, _name, _val) do {            \
     _stats_server_set_ts(_ctx, _server, STATS_SERVER_##_name, _val);   \
} while (0)

#else

#define stats_pool_incr(_ctx, _pool, _name)

#define stats_pool_decr(_ctx, _pool, _name)

#define stats_pool_incr_by(_ctx, _pool, _name, _val)

#define stats_pool_decr_by(_ctx, _pool, _name, _val)

#define stats_server_incr(_ctx, _server, _name)

#define stats_server_decr(_ctx, _server, _name)

#define stats_server_incr_by(_ctx, _server, _name, _val)

#define stats_server_decr_by(_ctx, _server, _name, _val)

#endif

#define stats_enabled   NC_STATS

void stats_describe(void);

void _stats_pool_incr(struct context *ctx, struct server_pool *pool, stats_pool_field_t fidx);
void _stats_pool_decr(struct context *ctx, struct server_pool *pool, stats_pool_field_t fidx);
void _stats_pool_incr_by(struct context *ctx, struct server_pool *pool, stats_pool_field_t fidx, int64_t val);
void _stats_pool_decr_by(struct context *ctx, struct server_pool *pool, stats_pool_field_t fidx, int64_t val);
void _stats_pool_set_ts(struct context *ctx, struct server_pool *pool, stats_pool_field_t fidx, int64_t val);

void _stats_server_incr(struct context *ctx, struct server *server, stats_server_field_t fidx);
void _stats_server_decr(struct context *ctx, struct server *server, stats_server_field_t fidx);
void _stats_server_incr_by(struct context *ctx, struct server *server, stats_server_field_t fidx, int64_t val);
void _stats_server_decr_by(struct context *ctx, struct server *server, stats_server_field_t fidx, int64_t val);
void _stats_server_set_ts(struct context *ctx, struct server *server, stats_server_field_t fidx, int64_t val);

struct stats *stats_create(uint16_t stats_port, char *stats_ip, int stats_interval, char *source, struct array *server_pool);
void stats_destroy(struct stats *stats);
void stats_swap(struct stats *stats);

#endif
