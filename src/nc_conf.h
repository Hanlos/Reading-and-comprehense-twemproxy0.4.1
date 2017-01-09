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

#ifndef _NC_CONF_H_
#define _NC_CONF_H_

#include <unistd.h>
#include <sys/types.h>
#include <sys/un.h>
#include <yaml.h>

#include <nc_core.h>
#include <hashkit/nc_hashkit.h>

#define CONF_OK             (void *) NULL
#define CONF_ERROR          (void *) "has an invalid value"

#define CONF_ROOT_DEPTH     1
#define CONF_MAX_DEPTH      CONF_ROOT_DEPTH + 1

#define CONF_DEFAULT_ARGS       3
#define CONF_DEFAULT_POOL       8
#define CONF_DEFAULT_SERVERS    8

#define CONF_UNSET_NUM  -1
#define CONF_UNSET_PTR  NULL
#define CONF_UNSET_HASH (hash_type_t) -1
#define CONF_UNSET_DIST (dist_type_t) -1

#define CONF_DEFAULT_HASH                    HASH_FNV1A_64
#define CONF_DEFAULT_DIST                    DIST_KETAMA //ketama
#define CONF_DEFAULT_TIMEOUT                 -1
#define CONF_DEFAULT_LISTEN_BACKLOG          512
#define CONF_DEFAULT_CLIENT_CONNECTIONS      0
#define CONF_DEFAULT_REDIS                   false
#define CONF_DEFAULT_REDIS_DB                0
#define CONF_DEFAULT_PRECONNECT              false
#define CONF_DEFAULT_AUTO_EJECT_HOSTS        false
#define CONF_DEFAULT_SERVER_RETRY_TIMEOUT    30 * 1000      /* in msec */
#define CONF_DEFAULT_SERVER_FAILURE_LIMIT    2
#define CONF_DEFAULT_SERVER_CONNECTIONS      1
#define CONF_DEFAULT_KETAMA_PORT             11211
#define CONF_DEFAULT_TCPKEEPALIVE            false

struct conf_listen {
    struct string   pname;   /* listen: as "hostname:port" */
    struct string   name;    /* hostname:port */
    int             port;    /* port */
    mode_t          perm;    /* socket permissions */
    struct sockinfo info;    /* listen socket info */
    unsigned        valid:1; /* valid? */
};

//������صĽṹ��·��instance->context->conf->conf_pool(conf_server)->server_pool(server)
struct conf_server { //������conf_add_server
    //- 127.0.0.1:11211:1 server1�е�- 127.0.0.1:11211:1
    struct string   pname;      /* server: as "hostname:port:weight" */
    //- 127.0.0.1:11211:1 server1�е�server1   
    //ע�������˷���������Ϊ- 127.0.0.1:11211:1��û��serverx�����ı�ʶ�����Ҷ˿�Ϊ11211����name�������11211:1  
    //��Զ����stats����ͻ���Ӱ�죬��conf_add_server
    struct string   name;       /* hostname:port or [name] */
    struct string   addrstr;    /* hostname */
    int             port;       /* port */
    int             weight;     /* weight */
    struct sockinfo info;       /* connect socket info */
    unsigned        valid:1;    /* valid? */
};

//������صĽṹ��·��instance->context->conf->conf_pool(conf_server)->server_pool(server)
//������������conf_handler  ,���մ��뵽conf_pool���ο�����conf_commands
//�����ļ���ÿһ��pool��Ӧ��������Ϣ   �洢��conf->pool��

//conf_pool��server_pool�кܶ�����һ���ģ�ΪʲôҪ��������?��Ϊconf_pool���������ļ���һ�ݿ�����
//��server_pool��������ͳ�Ƹ���ͳ����Ϣ�Լ���hash�ã����Բο�conf_dump

//server_pool�洢��context->pool   conf_pool�ṹ����conf->pool������  �ο�server_pool_init(&ctx->pool, &ctx->cf->pool, ctx);
struct conf_pool { //�ýṹ�����������Ĭ��ֵΪconf_validate_pool         conf_pool�ṹ����conf->pool������
    struct string      name;                  /* pool name (root node)  */
    //�ο�conf_commands  ������������conf_handler 
    struct conf_listen listen;                /* listen: */
    /*
    hash  ����ѡ���keyֵ��hash�㷨��
>one_at_a_time >md5 >crc16  
>crc32 (crc32 implementation compatible with libmemcached) 
>crc32a (correct crc32 implementation as per the spec) 
>fnv1_64 >fnv1a_64 >fnv1_32 >fnv1a_32 >hsieh >murmur >jenkins   ���ûѡ��Ĭ����fnv1a_64��
    */
    hash_type_t        hash;                  /* hash: */  //Ĭ��ֵCONF_DEFAULT_HASH
    /*
    ����һ����Ʒ�У���Ʒ������Ϣ(p:id:)����Ʒ����(d:id:)����ɫ����(c:id:)�ȣ��������Ǵ洢ʱ������HashTag��
    �ᵼ����Щ���ݲ���洢��һ����Ƭ�����Ƿ�ɢ�������Ƭ��������ȡʱ����Ҫ�Ӷ����Ƭ��ȡ���ݽ��кϲ����޷�
    ����mget����ô�������HashTag����ô����ʹ�á�::���м����������Ƭ�߼�������idһ���Ľ���ֵ�һ����Ƭ��
    */ //hash_tag: "{}"��ʾ����hashֵʱ��ֻȡkey�а�����{}������ǲ��������� 
    struct string      hash_tag;              /* hash_tag: */
    //����ketama��modula��random3�ֿ�ѡ������  ȡֵ��DIST_CODEC
    dist_type_t        distribution;          /* distribution: */
    //��λ�Ǻ��룬�����ӵ�server�ĳ�ʱֵ��Ĭ�������õȴ���  ��λms   //���������������timeout����λĬ����ms
    //�����˳�ʱѡ��᲻��Ժ�������������������������У��Ӷ��ر�����??????
    //��������ó�ʱʱ�䣬��twemproxy��redis�������쳣��twemproxy��ⲻ�������ͻ��˿��ܻ�һֱ�����ȴ�
    int                timeout;               /* timeout: */ //��������ã�Ĭ�����޴� CONF_DEFAULT_TIMEOUT
    //����TCP ��backlog�����ӵȴ����У��ĳ��ȣ�Ĭ����512��
    int                backlog;               /* backlog: */
    //��ֵ��proxy_accept���ӣ��ͻ�����������client_close_stats�м��١�
    int                client_connections;    /* client_connections: */
    //�Ƿ������ں�SO_KEEPALIVE
    int                tcpkeepalive;          /* tcpkeepalive: */ //���ո�ֵ��server_pool->tcpkeepalive����conf_pool_each_transform
    //�Ƿ����ӵ�redis
    int                redis;                 /* redis: */
    //���������redis_auth�������redis����Ϊtrue,��conf_validate_pool
    struct string      redis_auth;            /* redis_auth: redis auth password (matches requirepass on redis) */
    int                redis_db;              /* redis_db: redis db */ //Ĭ��db0
    //��һ��booleanֵ��ָʾtwemproxy�Ƿ�Ӧ��Ԥ����pool�е�server��Ĭ����false��
    int                preconnect;            /* preconnect: */
    //��һ��booleanֵ�����ڿ���twemproxy�Ƿ�Ӧ�ø���server������״̬�ؽ�Ⱥ�����������״̬����server_failure_limit��ֵ�����ơ�  Ĭ����false��
    //�Ƿ��ڽڵ�����޷���Ӧʱ�Զ�ժ���ýڵ�  ������Ч�ĵط��ں���server_failure��server_pool_update
    int                auto_eject_hosts;      /* auto_eject_hosts: */
    //ÿ��server���Ա��򿪵���������Ĭ�ϣ�ÿ����������һ�����ӡ�
    int                server_connections;    /* server_connections: */
    //��λ�Ǻ��룬���Ʒ��������ӵ�ʱ��������auto_eject_host������Ϊtrue��ʱ��������á�Ĭ����30000 ���롣
    //����ʱ�䣨���룩����������һ����ʱժ���Ĺ��Ͻڵ�ļ��������жϽڵ��������Զ��ӵ�һ����Hash����
    int                server_retry_timeout;  /* server_retry_timeout: in msec */
    //�������ӷ������Ĵ�������auto_eject_host������Ϊtrue��ʱ��������á�Ĭ����2��  �ڵ�����޷���Ӧ���ٴδ�һ����Hash����ʱժ������Ĭ����2
    int                server_failure_limit;  /* server_failure_limit: */ 
    /*
    һ��pool�еķ������ĵ�ַ���˿ں�Ȩ�ص��б�����һ����ѡ�ķ����������֣�����ṩ�����������֣�����ʹ��������server
    �Ĵ��򣬴Ӷ��ṩ��Ӧ��һ����hash��hash ring�����򣬽�ʹ��server������Ĵ���
    */ //����server_init conf_pool_each_transform������server_pool->server��
    struct array       server;                /* servers: conf_server[] */
    //��Ǹ�pool�Ƿ���Ч
    unsigned           valid:1;               /* valid? */
};

//������صĽṹ��·��instance->context->conf->conf_pool(conf_server)->server_pool(server)
//�ýṹ�����instance->cf��
struct conf { //�����ռ�ͳ�ʼ����conf_open   ���������洢��conf->pool�е������Աconf_pool��
    //-c����ָ�������ļ�·��
    char          *fname;           /* file name (ref in argv[]) */
    //��fname�ļ��������ļ����
    FILE          *fh;              /* file handle */ 
    struct array  arg;              /* string[] (parsed {key, value} pairs) */

    /*
    alpha:
  listen: 127.0.0.1:22121
  hash: fnv1a_64
  distribution: ketama
  auto_eject_hosts: true
  redis: true
  server_retry_timeout: 2000
  server_failure_limit: 1
  servers:
   - 127.0.0.1:6379:1

beta:
  listen: 127.0.0.1:22122
  hash: fnv1a_64
  hash_tag: "{}"
  distribution: ketama
  auto_eject_hosts: false
  timeout: 400
  redis: true
  servers:
   - server1 127.0.0.1:6380:1
   - server2 127.0.0.1:6381:1
   - server3 127.0.0.1:6382:1
   - server4 127.0.0.1:6383:1
    */ //alpha��beta���Զ�Ӧһ��conf_pool�ṹ
    //��Ա����conf_pool  ���������ô���������Աconf_pool��
    struct array  pool;             /* conf_pool[] (parsed pools) */  
    uint32_t      depth;            /* parsed tree depth */
    yaml_parser_t parser;           /* yaml parser */
    yaml_event_t  event;            /* yaml event */
    yaml_token_t  token;            /* yaml token */
    unsigned      seq:1;            /* sequence? */
    unsigned      valid_parser:1;   /* valid parser? */
    unsigned      valid_event:1;    /* valid event? */
    unsigned      valid_token:1;    /* valid token? */
    unsigned      sound:1;          /* sound? */
    unsigned      parsed:1;         /* parsed? */
    unsigned      valid:1;          /* valid? */
};

struct command {
    struct string name;
    char          *(*set)(struct conf *cf, struct command *cmd, void *data);
    int           offset;
};

#define null_command { null_string, NULL, 0 }

char *conf_set_string(struct conf *cf, struct command *cmd, void *conf);
char *conf_set_listen(struct conf *cf, struct command *cmd, void *conf);
char *conf_add_server(struct conf *cf, struct command *cmd, void *conf);
char *conf_set_num(struct conf *cf, struct command *cmd, void *conf);
char *conf_set_bool(struct conf *cf, struct command *cmd, void *conf);
char *conf_set_hash(struct conf *cf, struct command *cmd, void *conf);
char *conf_set_distribution(struct conf *cf, struct command *cmd, void *conf);
char *conf_set_hashtag(struct conf *cf, struct command *cmd, void *conf);

rstatus_t conf_server_each_transform(void *elem, void *data);
rstatus_t conf_pool_each_transform(void *elem, void *data);

struct conf *conf_create(char *filename);
void conf_destroy(struct conf *cf);

#endif
