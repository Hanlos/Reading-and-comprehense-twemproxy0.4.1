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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <nc_core.h>
#include <nc_server.h>
#include <nc_hashkit.h>

//��ketama���㷨����������ϴ�����������*160���㣬��Щ��ѻ��ֳ���ͬ�������ĶΡ�
#define KETAMA_CONTINUUM_ADDITION   10  /* # extra slots to build into continuum */
#define KETAMA_POINTS_PER_SERVER    160 /* 40 points per hash */
#define KETAMA_MAX_HOSTLEN          86

//����ĳ��������ĳ��point��hashֵ
//alignment��ֵ�̶���4��ketama_hash�Ƕ���server��+������ɵ�md5ǩ�����ӵ�16λ��ʼȡֵ��������һ��32λֵ��
static uint32_t
ketama_hash(const char *key, size_t key_length, uint32_t alignment) //YANG ADD XXXXXXXXXX TODO������������Ҫ��Ϊ��redisһ��
{
    unsigned char results[16];

    md5_signature((unsigned char*)key, key_length, results);

    return ((uint32_t) (results[3 + alignment * 4] & 0xFF) << 24)
        | ((uint32_t) (results[2 + alignment * 4] & 0xFF) << 16)
        | ((uint32_t) (results[1 + alignment * 4] & 0xFF) << 8)
        | (results[0 + alignment * 4] & 0xFF);
}

//�Ƚ�������������ֵ��������ketama_update ����������
static int
ketama_item_cmp(const void *t1, const void *t2)
{
    const struct continuum *ct1 = t1, *ct2 = t2;

    if (ct1->value == ct2->value) {
        return 0;
    } else if (ct1->value > ct2->value) {
        return 1;
    } else {
        return -1;
    }
}

/*
ketama��һ����hash�㷨��ʵ��˼·�ǣ�
(1) ͨ�������ļ�������һ���������б�����ʽ�磺(1.1.1.1:11211, 2.2.2.2:11211,9.8.7.6:11211...)
(2) ��ÿ���������б��е��ַ�����ͨ��Hash�㷨��hash�ɼ����޷�����������
    ע�⣺���ͨ��hash�㷨�������أ�
(3) ���⼸���޷����������ŵ�һ�����ϣ����������Ϊcontinuum�������ǿ�������һ����0��2^32���ӱ�
(4) ���Խ���һ�����ݽṹ����ÿ�����ͷ�������ip��ַ��Ӧ��һ��������ÿ���������ͳ�����������ϵ��⼸��λ���ϡ�
    ע�⣺�⼸�������������ŷ����������Ӻ�ɾ�����仯���������ܱ�֤��Ⱥ����/ɾ����������ǰ����Щkey��ӳ�䵽ͬ����ip��ַ�ϡ����潫����ϸ˵����ô����
(5) Ϊ�˰�һ��keyӳ�䵽һ���������ϣ���Ҫ��key��hash���γ�һ���޷���������un��Ȼ���ڻ�continuum�ϲ��Ҵ���un����һ����ֵ�����ҵ����Ͱ�key���浽��̨�������ϡ�
(6) �����hash(key)ֵ����continuum�ϵ��������ֵ����ֱ�ӻ��ĵ�continuum���Ŀ�ʼλ�á�
    ��������ӻ�ɾ����Ⱥ�еĽ�㣬��ֻ��Ӱ��һ�ٲ���key�ķֲ���
    ע�⣺����˵�Ļ�Ӱ��һ����key����Եġ���ʵӰ���key�Ķ��٣��ɸ�ip��ַռ��Ȩ�ش�С�����ġ���ketama
    �������ļ��У���Ҫָ��ÿ��ip��ַ��Ȩ�ء�Ȩ�ش���ڻ���ռ�ĵ�Ͷࡣ
*/
//ketamaһ����hash�ο�:http://www.cnblogs.com/basecn/p/4288456.html  http://blog.chinaunix.net/uid-20498361-id-4303232.html
//�ú����Ǵ���continuum�ĺ��ĺ��������ȴ������ļ��ж�ȡ��Ⱥ������ip�Ͷ˿ڣ��Լ�Ȩ����Ϣ������continuum����������Щ��������Ϣ�ͻ��ϵ������±��Ӧ������


/*
����������Ϣ�еķ�����Ȩ�أ������ڻ��ϵĵ�ṹ����������������Ȩ��ֵ�ֱ�Ϊ1 2 3���򴴽��Ķ�Ӧ�ڻ�(����ȡֵ��Χ��0-0XFFFFFFFF)��
��struct continuum�ṹ��ĿΪ(160*3) * (1/6),(160*3) * (2/6), (160*3) * (3/6),Ȼ��ͨ��server���ؼ�����hash���㣬����ڻ��ϵ�
(160*3) * (1/6),(160*3) * (2/6), (160*3) * (3/6)�����Ӧ��λ�ã�һ�㶼�ǱȽϾ��ȵķֲ�������struct continuum��Ӧ��value(Ҳ�����ڻ��ϵ�λ��)
��������
*/
rstatus_t  //����server-pool�ķ������
ketama_update(struct server_pool *pool) //server_pool_server����ѡ�ٺ�˷�������ketama_updateΪһ����hash��ص�ketama�㷨
{
    // �ñ�������¼���������ļ��й���ȡ�˶��ٸ�������
    uint32_t nserver;             /* # server - live and dead */
    //��Ч����������
    uint32_t nlive_server;        /* # live server */
    //�����������Ȩ�ظպ�ƽ��ֵ�Ļ������ֵ����160��
    uint32_t pointer_per_server;  /* pointers per server proportional to weight */
    uint32_t pointer_per_hash;    /* pointers per hash */
    uint32_t pointer_counter;     /* # pointers on continuum */
    uint32_t pointer_index;       /* pointer index */
    uint32_t points_per_server;   /* points per server */ //KETAMA_POINTS_PER_SERVER
    //�޷����������ŵ�һ�����ϣ����������Ϊcontinuum
    uint32_t continuum_index;     /* continuum index */
    uint32_t continuum_addition;  /* extra space in the continuum */
    uint32_t server_index;        /* server index */
    uint32_t value;               /* continuum value */
    // �ñ����������ļ������з�����Ȩ��ֵ���ܺ�
    uint32_t total_weight;        /* total live server weight */
    int64_t now;                  /* current timestamp in usec */

    ASSERT(array_n(&pool->server) > 0);

    now = nc_usec_now();
    if (now < 0) {
        return NC_ERROR;
    }

    /*
     * Count live servers and total weight, and also update the next time to
     * rebuild the distribution
     */
    nserver = array_n(&pool->server);
    nlive_server = 0;
    total_weight = 0; //���з�������Ȩ���ܺ�
    pool->next_rebuild = 0LL;
    for (server_index = 0; server_index < nserver; server_index++) {
        struct server *server = array_get(&pool->server, server_index);

        if (pool->auto_eject_hosts) {
            if (server->next_retry <= now) {
                server->next_retry = 0LL;
                nlive_server++;
            } else if (pool->next_rebuild == 0LL ||
                       server->next_retry < pool->next_rebuild) {
                pool->next_rebuild = server->next_retry;
            }
        } else {
            nlive_server++;
        }

        ASSERT(server->weight > 0);

        /* count weight only for live servers */
        if (!pool->auto_eject_hosts || server->next_retry <= now) {
            total_weight += server->weight;
        }
    }

    pool->nlive_server = nlive_server;

    if (nlive_server == 0) {
        log_debug(LOG_DEBUG, "no live servers for pool %"PRIu32" '%.*s'",
                  pool->idx, pool->name.len, pool->name.data);

        return NC_OK;
    }
    log_debug(LOG_DEBUG, "%"PRIu32" of %"PRIu32" servers are live for pool "
              "%"PRIu32" '%.*s'", nlive_server, nserver, pool->idx,
              pool->name.len, pool->name.data);

    continuum_addition = KETAMA_CONTINUUM_ADDITION;
    points_per_server = KETAMA_POINTS_PER_SERVER;
    /*
     * Allocate the continuum for the pool, the first time, and every time we
     * add a new server to the pool
     */
    if (nlive_server > pool->nserver_continuum) {
        struct continuum *continuum;
        uint32_t nserver_continuum = nlive_server + continuum_addition; //ʵ�����߷������� + 10
        //��ketama���㷨����������ϴ�����������*160���㣬��Щ��ѻ��ֳ���ͬ�������ĶΡ�
        uint32_t ncontinuum = nserver_continuum * points_per_server;//*160

        continuum = nc_realloc(pool->continuum, sizeof(*continuum) * ncontinuum); //ʵ��������������continuum_addition * 160��continuum�ṹ��Ϊʲô��?????
        if (continuum == NULL) {
            return NC_ENOMEM;
        }

        pool->continuum = continuum; //һ�������˶��ٸ�struct continuum�ṹ��(ʵ�ʵķ�������+10)*160
        pool->nserver_continuum = nserver_continuum;  //һ����hash�����Ч��������+ KETAMA_CONTINUUM_ADDITION
        /* pool->ncontinuum is initialized later as it could be <= ncontinuum */
    }

    /*
     * Build a continuum with the servers that are live and points from
     * these servers that are proportial to their weight
     */
    continuum_index = 0;
    pointer_counter = 0;
    for (server_index = 0; server_index < nserver; server_index++) {
        struct server *server;
        float pct;

        server = array_get(&pool->server, server_index);

        if (pool->auto_eject_hosts && server->next_retry > now) {
            continue;
        }

        pct = (float)server->weight / (float)total_weight;
        //floorf ����С�ڻ��ߵ���ָ�����ʽ����������� 
        //����Ȩ�ؼ���ĳ����������Ӧ��ӵ�еĻ�����������Ȩ��Խ�󣬵���Խ��
        pointer_per_server = (uint32_t) ((floorf((float) (pct * KETAMA_POINTS_PER_SERVER / 4 * (float)nlive_server + 0.0000000001))) * 4);
        pointer_per_hash = 4;

/* ���ö�Ӧ�Ĵ�ӡ
servers:
 - 192.168.1.111:7000:1 server1
 - 192.168.1.111:7001:1 server2
 - 192.168.1.111:7002:1 server3

[2016-06-06 15:02:05.946] nc_ketama.c:188 server1 weight 1 of 3 pct 0.33333 points per server 160
[2016-06-06 15:02:05.946] nc_ketama.c:188 server2 weight 1 of 3 pct 0.33333 points per server 160
[2016-06-06 15:02:05.946] nc_ketama.c:188 server3 weight 1 of 3 pct 0.33333 points per server 160
*/
        log_debug(LOG_VERB, "%.*s weight %"PRIu32" of %"PRIu32" "
                  "pct %0.5f points per server %"PRIu32"",
                  server->name.len, server->name.data, server->weight,
                  total_weight, pct, pointer_per_server);

        for (pointer_index = 1;
             pointer_index <= pointer_per_server / pointer_per_hash;
             pointer_index++) {

            char host[KETAMA_MAX_HOSTLEN]= "";
            size_t hostlen;
            uint32_t x;

            hostlen = snprintf(host, KETAMA_MAX_HOSTLEN, "%.*s-%u",
                               server->name.len, server->name.data,
                               pointer_index - 1);

            for (x = 0; x < pointer_per_hash; x++) { //ƽ��һ���������ڻ��ϻ���160���㣬�������Ȩ�ر�ֵ������ͬ��Ȩ��Խ���ڻ��ϵĵ���Խ��
                value = ketama_hash(host, hostlen, x); 
                pool->continuum[continuum_index].index = server_index; //�ڻ��ϵ�valueֵ������Ӧ�ĺ��server
                pool->continuum[continuum_index++].value = value; //�ڻ��ϵ�value
            }
        }
        pointer_counter += pointer_per_server; //�������к�˷������ڻ������ж��ٸ���
    }

    pool->ncontinuum = pointer_counter; //�������к�˷������ڻ������ж��ٸ���,����ֵ��ʵ�ʷ����continuumҪ��10*160��
    qsort(pool->continuum, pool->ncontinuum, sizeof(*pool->continuum),
          ketama_item_cmp); //�����ڻ��ϵ�value�����������е�continuum��������

    for (pointer_index = 0;
         pointer_index < ((nlive_server * KETAMA_POINTS_PER_SERVER) - 1);
         pointer_index++) {
        if (pointer_index + 1 >= pointer_counter) {
            break;
        }
        ASSERT(pool->continuum[pointer_index].value <=
               pool->continuum[pointer_index + 1].value);
    }

    //updated pool 0 'beta' with 3 of 3 servers live in 13 slots and 480 active points in 3680 slots
    log_debug(LOG_VERB, "updated pool %"PRIu32" '%.*s' with %"PRIu32" of "
              "%"PRIu32" servers live in %"PRIu32" slots and %"PRIu32" "
              "active points in %"PRIu32" slots", pool->idx,
              pool->name.len, pool->name.data, nlive_server, nserver,
              pool->nserver_continuum, pool->ncontinuum,
              (pool->nserver_continuum + continuum_addition) * points_per_server);

    return NC_OK;
}

//�ҳ�����hashֵ���ڵ�������   ʹ�ö��ַ��ҵ�һ��ֵ�ڻ��еĶ�Ӧ����
uint32_t
ketama_dispatch(struct continuum *continuum, uint32_t ncontinuum, uint32_t hash)
{
    struct continuum *begin, *end, *left, *right, *middle;

    ASSERT(continuum != NULL);
    ASSERT(ncontinuum != 0);

    begin = left = continuum;
    end = right = continuum + ncontinuum;

    while (left < right) {
        middle = left + (right - left) / 2;
        if (middle->value < hash) {
          left = middle + 1;
        } else {
          right = middle;
        }
    }

    if (right == end) {
        right = begin;
    }

    return right->index;
}

