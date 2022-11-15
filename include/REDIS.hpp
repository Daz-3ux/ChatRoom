#ifndef _SE_REDIS_H
#define _SE_REDIS_H

#include <hiredis/hiredis.h>

#include <cstring>
#include <iostream>
#include <list>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "error.h"

class Redis {
 public:
  Redis() {
    m_redis = NULL;
    init();
  }
  ~Redis() {
    if (m_redis != NULL) {
      redisFree(m_redis);
      std::cout << "redis存储完毕" << std::endl;
    }
  }

  bool setString(std::string key, std::string value) {  // SET
    redisReply *reply;
    bool result = false;
    reply = (redisReply *)redisCommand(m_redis, "SET %s %s", key.c_str(),
                                       value.c_str());
    if (reply == NULL) {
      redisFree(m_redis);
      m_redis = NULL;
      result = false;
      std::cout << "set string faild" << __LINE__ << std::endl;
      return result;
    } else if (strcmp(reply->str, "OK") == 0) {
      result = true;
    }

    freeReplyObject(reply);

    return result;
  }

  // std::string getString(std::string key) {  // GET
  //   redisReply *reply;
  //   reply = (redisReply *)redisCommand(m_redis, "GET %s", key.c_str());
  //   if (reply == nullptr) {
  //     redisFree(m_redis);
  //     m_redis = nullptr;
  //     std::cout << "get nothing" << std::endl;
  //     return nullptr;
  //   } else if (reply->len <= 0) {
  //     freeReplyObject(reply);
  //     return nullptr;
  //   } else {
  //     //std::stringstream ss;
  //     std::string ss = reply->str;
  //     //ss << reply->str;
  //     freeReplyObject(reply);
  //     //return ss.str();
  //     return ss;
  //   }
  // }

  std::string getString(std::string key) {  // GET
    redisReply *reply;
    reply = (redisReply *)redisCommand(m_redis, "GET %s", key.c_str());
    std::string s = "";
    if (reply == NULL) {
      return s;
    }
    std::stringstream ss;
    ss << reply->str;
    freeReplyObject(reply);
    return ss.str();
  }

  bool delKey(std::string key) {  // DEL
    redisReply *reply;
    reply = (redisReply *)redisCommand(m_redis, "DEL %s", key.c_str());
    if (reply == NULL) {
      redisFree(m_redis);
      m_redis = NULL;
      return false;
    }
    return true;
  }

  bool setVector(std::string key, std::vector<int> value) {  // RPUSH
    redisReply *reply;

    int valueSize = value.size();
    bool result = false;

    for (int i = 0; i < valueSize; i++) {
      reply = (redisReply *)redisCommand(m_redis, "RPUSH %s %d", key.c_str(),
                                         value.at(i));

      if (reply == NULL) {
        redisFree(m_redis);
        m_redis = NULL;
        result = false;
        std::cout << "set list fail" << std::endl;
        return result;
      }
    }
    freeReplyObject(reply);
    return result;
  }

  std::vector<int> getVector(std::string key) {  // LRANGE
    redisReply *reply;
    reply = (redisReply *)redisCommand(m_redis, "LLEN %s", key.c_str());
    int valueSize = reply->integer;

    reply = (redisReply *)redisCommand(m_redis, "LRANGE %s %d %d", key.c_str(),
                                       0, valueSize - 1);

    redisReply **replyVector = reply->element;
    std::vector<int> result;
    for (int i = 0; i < valueSize; i++) {
      std::string temp = (*replyVector)->str;
      int a = atoi(temp.c_str());
      result.push_back(a);
      replyVector++;
    }
    return result;
  }

  bool setHash(std::string key, std::string field, std::string value) {  // HSET
    redisReply *reply;
    reply = (redisReply *)redisCommand(m_redis, "HSET %s %s %s", key.c_str(),
                                       field.c_str(), value.c_str());
    if (reply == NULL) {
      redisFree(m_redis);
      m_redis = NULL;
      return false;
    }
    return true;
  }

  bool setHashFd(std::string key, std::string field, int value) {  // HSET
    redisReply *reply;
    reply = (redisReply *)redisCommand(m_redis, "HSET %s %s %d", key.c_str(),
                                       field.c_str(), value);
    if (reply == NULL) {
      redisFree(m_redis);
      m_redis = NULL;
      return false;
    }
    return true;
  }

  int hashExist(std::string key, std::string field) {  // HEXISTS
    redisReply *reply;
    reply = (redisReply *)redisCommand(m_redis, "HEXISTS %s %s", key.c_str(),
                                       field.c_str());
    if (reply == NULL) {
      redisFree(m_redis);
      m_redis = NULL;
      return -1;
    }
    return reply->integer;
  }

  std::string getHash(std::string key, std::string field) {  // HGET
    redisReply *reply;
    reply = (redisReply *)redisCommand(m_redis, "HGET %s %s", key.c_str(),
                                       field.c_str());
    if (reply == NULL) {
      redisFree(m_redis);
      m_redis = NULL;
      return nullptr;
    }
    return reply->str;
  }

  std::vector<std::string> getHashKey(std::string key) {  // HKEYS
    redisReply *reply;
    reply = (redisReply *)redisCommand(m_redis, "HLEN %s", key.c_str());
    int size = reply->integer;

    reply = (redisReply *)redisCommand(m_redis, "HKEYS %s", key.c_str());
    redisReply **replyVector = reply->element;
    std::vector<std::string> result;
    for (int i = 0; i < size; i++) {
      std::string temp = (*replyVector)->str;
      result.push_back(temp);
      replyVector++;
    }
    return result;
  }

  int getHashFd(std::string key, std::string field) {  // HGET
    redisReply *reply;
    reply = (redisReply *)redisCommand(m_redis, "HGET %s %s", key.c_str(),
                                       field.c_str());
    if (reply == NULL) {
      redisFree(m_redis);
      m_redis = NULL;
      return -1;
    }
    return reply->integer;
  }

  bool hashDel(std::string key, std::string field) {  // HDEL
    redisReply *reply;
    reply = (redisReply *)redisCommand(m_redis, "HDEL %s %s", key.c_str(),
                                       field.c_str());
    if (reply == NULL) {
      redisFree(m_redis);
      m_redis = NULL;
      return false;
    }
    return true;
  }

  bool saddValue(
      std::string key,
      std::string
          value) {  // sadd:将一个或多个成员元素加入到集合中，已经存在于集合的成员元素将被忽略
    redisReply *reply;
    reply = (redisReply *)redisCommand(m_redis, "SADD %s %s", key.c_str(),
                                       value.c_str());
    if (reply == NULL) {
      redisFree(m_redis);
      m_redis = NULL;
      return false;
    }
    return true;
  }

  int sismember(std::string key, std::string value) {  // SISMEMBER
    redisReply *reply;
    reply = (redisReply *)redisCommand(m_redis, "SISMEMBER %s %s", key.c_str(),
                                       value.c_str());
    if (reply == NULL) {
      redisFree(m_redis);
      m_redis = NULL;
      return -1;
    }
    return reply->integer;
  }

  bool srmmember(std::string key, std::string value) {  // SREM
    redisReply *reply;
    reply = (redisReply *)redisCommand(m_redis, "SREM %s %s", key.c_str(),
                                       value.c_str());
    if (reply == NULL) {
      redisFree(m_redis);
      m_redis = NULL;
      return false;
    }
    return true;
  }

  int hlen(std::string key) {  // HLEN
    redisReply *reply;
    reply = (redisReply *)redisCommand(m_redis, "HLEN %s", key.c_str());
    if (reply == NULL) {
      redisFree(m_redis);
      m_redis = NULL;
      return -1;
    }
    return reply->integer;
  }

  int slen(std::string key) {  // SCARD:返回集合中元素的数量
    redisReply *reply;
    reply = (redisReply *)redisCommand(m_redis, "SCARD %s", key.c_str());
    if (reply == NULL) {
      redisFree(m_redis);
      m_redis = NULL;
      return -1;
    }
    return reply->integer;
  }

  std::vector<std::string> smembers(
      std::string key) {  // SMEMBERS: 返回集合中所有成员
    redisReply *reply;
    reply = (redisReply *)redisCommand(m_redis, "SCARD %s", key.c_str());
    int size = reply->integer;
    reply = (redisReply *)redisCommand(m_redis, "SMEMBERS %s", key.c_str());
    redisReply **replyVector = reply->element;
    std::vector<std::string> result;
    for (int i = 0; i < size; i++) {
      std::string temp = (*replyVector)->str;
      result.push_back(temp);
      replyVector++;
    }
    return result;
  }

  int lpush(std::string key,
            std::string value) {  // LPUSH:将一个或多个值插入到列表头部
    // 如果 key 不存在，一个空列表会被创建并执行 LPUSH 操作
    // 当 key 存在但不是列表类型时，返回一个错误
    redisReply *reply;
    reply = (redisReply *)redisCommand(m_redis, "LPUSH %s %s", key.c_str(),
                                       value.c_str());
    if (reply == NULL) {
      redisFree(m_redis);
      m_redis = NULL;
      return false;
    }
    return true;
  }

  int llen(std::string key) {  // LLEN: 返回列表长度
    redisReply *reply;
    reply = (redisReply *)redisCommand(m_redis, "LLEN %s", key.c_str());
    if (reply == NULL) {
      redisFree(m_redis);
      m_redis = NULL;
      return -1;
    }
    return reply->integer;
  }

  redisReply **lrange(std::string key)  //返回消息链表所有元素
  {                                     // LRANGE
    redisReply *reply;
    // 0 为第一个元素, -1 为最后一个元素
    reply = (redisReply *)redisCommand(m_redis, "LRANGE %s %d %d", key.c_str(),
                                       0, -1);
    if (reply == NULL) {
      redisFree(m_redis);
      m_redis = NULL;
      return nullptr;
    }
    return reply->element;
  }

  redisReply **lrange(std::string key, int a, int b)  //返回指定的消息记录
  {                                                   // LRANGE
    redisReply *reply;
    reply = (redisReply *)redisCommand(m_redis, "LRANGE %s %d %d", key.c_str(),
                                       a, b);
    if (reply == NULL) {
      redisFree(m_redis);
      m_redis = NULL;
      return nullptr;
    }
    return reply->element;
  }

  bool ltrim(std::string key)  //删除链表中的所有元素
  {                            // lTRIM
    redisReply *reply;
    reply = (redisReply *)redisCommand(m_redis, "LTRIM %s %d %d", key.c_str(),
                                       0, -1);
    if (reply == NULL) {
      redisFree(m_redis);
      m_redis = NULL;
      return false;
    }
    return true;
  }

  bool lrem(std::string key, std::string value) {  // LREM
    redisReply *reply;
    reply = (redisReply *)redisCommand(m_redis, "LREM %s %d %s", key.c_str(), 0,
                                       value.c_str());
    if (reply == NULL) {
      redisFree(m_redis);
      m_redis = NULL;
      return false;
    }
    return true;
  }

  std::string lpop(std::string key) {  // LPOP
    redisReply *reply;
    reply = (redisReply *)redisCommand(m_redis, "LPOP %s", key.c_str());
    if (reply == NULL) {
      redisFree(m_redis);
      m_redis = NULL;
      return nullptr;
    }
    return reply->str;
  }

 private:
  void init() {
    struct timeval timeout = {1, 50000};  // 连接等待时间为1.5秒
    m_redis = redisConnectWithTimeout("127.0.0.1", 6379, timeout);
    if (m_redis->err) {
      my_error("RedisTool : Connection error", __FILE__, __LINE__);
    } else {
      std::cout << "init redis success" << std::endl;
    }
  }

  redisContext *m_redis;
};

#endif