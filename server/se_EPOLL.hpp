#ifndef _SE_EPOLL_H
#define _SE_EPOLL_H
#include <dirent.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <list>

#include "../include/IO.hpp"
#include "../include/REDIS.hpp"
#include "../include/SOCK.hpp"
#include "../include/error.h"
#include "se_JSON.hpp"
#include "se_USER.hpp"

typedef struct socketinfo {
  int fd;
  int epfd;
} socketInfo;

std::string userMap("userMap");
User userCtl;

int flag_exit = 1; // 防止私聊exit后二次退出线程

class Epoll {
public:
  static int Create() {
    int epfd = epoll_create(6);
    if (epfd < 0) {
      my_error("epoll_create", __FILE__, __LINE__);
    }
    return epfd;
  }

  static void reset_oneshot(int epfd, int fd) {
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLERR | EPOLLONESHOT;
    epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &event);
  }
};

void *acceptConn(void *arg) {
  // printf("accept connection: %ld", pthread_self());
  socketInfo *info = (socketInfo *)arg;
  int cfd = accept(info->fd, NULL, NULL);

  int flag = fcntl(cfd, F_GETFL);
  flag |= O_NONBLOCK;
  fcntl(cfd, F_SETFL, flag);

  // 将客户连接加入至epoll
  struct epoll_event ev;
  ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLERR | EPOLLONESHOT;
  ev.data.fd = cfd;
  int ret = epoll_ctl(info->epfd, EPOLL_CTL_ADD, cfd, &ev);
  if (ret == -1) {
    my_error("ctl error", __FILE__, __LINE__);
    exit(1);
  }

  free(info);
  return NULL;
}

static int fileflag = 0;
static std::string myfilename;
static __off_t mysize = 0;
static int myclifd;

void *communication(void *arg) {
  printf("communication: %ld\n", pthread_self());
  socketInfo *info = (socketInfo *)arg;
  int fd = info->fd;
  int epfd = info->epfd;
  char buf[4096];
  memset(buf, 0, sizeof(buf));
  // std::cout << "flag = " << fileflag << std::endl;

  while (1) {
    if (fileflag) {  // 接受文件到服务器并保存
      std::cout << "接收1" << std::endl;
      sleep(1);
      DIR *dir = opendir("../tempfiles");
      if (dir == nullptr) {
        mkdir("../tempfiles", 0777);
      }
      FILE *fp = fopen(myfilename.c_str(), "w");
      char buf[8192];
      int n = 0;
      while (mysize > 0) {
        std::cout << "接收2" << std::endl;
        std::cout << mysize << std::endl;
        std::cout << myclifd << std::endl;
        if (sizeof(buf) < mysize) {
          n = IO::Readn(myclifd, buf, sizeof(buf));
        } else {
          n = IO::Readn(myclifd, buf, mysize);
        }
        std::cout << "n:  " << n << std::endl;
        // sleep(1);
        if (n < 0) {
          continue;
        }
        mysize -= n;
        fwrite(buf, n, 1, fp);
        std::cout << "size = " << mysize << std::endl;
      }
      sleep(1);
      fclose(fp);

      fileflag = 0;
    }
    int ret = IO::RecvMsg(fd, buf, sizeof(buf));
    if (ret == 0) {
      userCtl.delInList2(fd);
      close(fd);
      std::cout << "有一个客户端离线啦" << std::endl;
      break;
    } else if (ret < 0) {
      if (errno == EAGAIN) {
        Epoll::reset_oneshot(epfd, fd);
        break;
      }
    }

    std::string s(buf, strlen(buf));

    std::cout << s << std::endl;

    json js = json::parse(s);

    int status = js["loginStatus"];
    std::cout << "loginstatus: " << status << std::endl;

    Redis myredis;

    if (status == 1) {  // 注册
      int flag = myredis.hashExist(userMap, js["name"]);
      if (flag == 0) {
        bool ret = myredis.setHash(userMap, js["name"], s);
        if (ret) {
          IO::SendMsg(fd, "Success:注册成功", 21);
        } else {
          IO::SendMsg(fd, "Faild:注册失败", 19);
        }
      } else {
        IO::SendMsg(fd, "用户已注册", 16);
      }
    } else if (status == 2) {  // 登陆
      std::cout << "让我们来登陆吧" << std::endl;
      bool i = userCtl.cycleFind(js["name"]);
      if (i != false) {
        IO::SendMsg(fd, "already", 8);
      } else {
        int hashExist = myredis.hashExist(userMap, js["name"]);
        std::cout << hashExist << std::endl;
        if (hashExist) {
          std::string user = myredis.getHash(userMap, js["name"]);
          json js_temp = json::parse(user);
          std::string pass = js_temp["passwd"];
          std::string realpass = js["passwd"];
          if (strcmp(pass.c_str(), realpass.c_str()) == 0) {
            userCtl.addInList(js["name"], fd);
            IO::SendMsg(fd, "Success", 8);
          } else {
            IO::SendMsg(fd, "Faild", 6);
          }
        } else {
          IO::SendMsg(fd, "Faild", 6);
        }
      }
    } else if (status == 3) {  // 修改密码
      std::cout << "让我们来修改密码吧" << std::endl;
      int hashExist = myredis.hashExist(userMap, js["name"]);
      if (hashExist) {
        std::string user = myredis.getHash(userMap, js["name"]);
        json js_temp = json::parse(user);

        if (js["oldPasswd"] == js_temp["passwd"]) {
          js_temp["passwd"] = js["newPasswd"];
          std::string withNewPasswd = js_temp.dump();
          myredis.setHash(userMap, js["name"], withNewPasswd);
          IO::SendMsg(fd, "Success", 8);
        } else {
          IO::SendMsg(fd, "Faild_2", 8);
        }
      } else {
        IO::SendMsg(fd, "Faild_1", 8);
      }
    } else if (status == 5) {  // 修改昵称
      std::cout << "让我们来修改昵称吧" << std::endl;
    } else if (status == 4) {  // 找回密码
      std::cout << "让我们来找回密码吧" << std::endl;

    } else if (status == 6) {  // 退出登陆
      std::cout << "让我们退出登陆吧" << std::endl;
      // userCtl.cycle();
      userCtl.delInList(js["name"]);
      //close(fd);
      std::cout << "用户退出在线链表: " << js["name"] << std::endl;

    } else if (status == 7) {  // 添加好友
      std::cout << "让我们来添加好友吧" << std::endl;

      std::string a = js["myName"];
      std::string b("_friend");
      std::string st = a + b;
      std::string che_a = js["nameWant"];
      std::string che_b("_friendWant");
      std::string che_c = che_a + che_b;
      if (myredis.hashExist(st, js["nameWant"])) {
        IO::SendMsg(fd, "已经是好友啦!", 20);
      } else {
        if (myredis.hashExist(userMap, js["nameWant"])) {
          if (myredis.hashExist(che_c, js["nameWant"])) {
            IO::SendMsg(fd, "Faild:重复申请", 19);
          } else {
            std::string a = js["nameWant"];
            std::string b("_friApply");
            std::string str = a + b;
            myredis.lpush(str, js["myName"]);

            std::string ding("apply_ding");
            std::string dd = a + ding;
            myredis.lpush(dd, js["myName"]);

            IO::SendMsg(fd, "Ok", 3);
          }
        } else {
          IO::SendMsg(fd, "Faild:该用户不存在", 25);
        }
      }
    } else if (status == 8) {  // 查看好友申请列表
      std::cout << "让我们来查看申请列表吧" << std::endl;
      int len =  myredis.llen(js["name"]);
      if(len == 0) {
        IO::SendMsg(fd, "empty", 6);
      }else {
        std::string length = std::to_string(len);
        IO::SendMsg(fd, length.c_str(), length.size());
        for(int i = 0; i < len; i++) {
          std::string name = myredis.lpop(js["name"]);
          IO::SendMsg(fd, name.c_str(), name.size()+1);
        }
      }

    } else if (status == 9) {  // 同意好友申请
      std::cout << "让我们将好友放入表中" << std::endl;
      std::string a = js["myName"];
      std::string b("_friend");
      std::string str = a + b;
      std::string aa = js["nameWant"];
      std::string strr = aa+b;
      myredis.setHash(str, js["nameWant"], "true");
      myredis.setHash(strr, js["myName"], "true");
    } else if (status == 10) {  // 查看好友列表
      std::cout << "让我们来查看好友列表" << std::endl;
      std::string a = js["name"];
      std::string b("_friend");
      std::string str = a + b;
      std::vector<std::string> aa = myredis.getHashKey(str);
      int len = aa.size();
      // std::cout << len << std::endl;
      std::string length = std::to_string(len);
      IO::SendMsg(fd, length.c_str(), length.size());

      for (int i = 0; i < len; i++) {
        if (userCtl.cycleFind(aa[i])) {
          std::string online("[online]");
          std::string str = aa[i] + online;
          IO::SendMsg(fd, str.c_str(), str.size()+1);
        } else {
          std::string offline("[offline]");
          std::string str = aa[i] + offline;
          IO::SendMsg(fd, str.c_str(), str.size()+1);
        }
        //sleep(0.2);
      }
    } else if (status == 11) {  // 删除好友
      std::cout << "让我们来删除指定好友" << std::endl;
      std::string a = js["myName"];
      std::string b("_friend");
      std::string str = a + b;
      std::string aa = js["nameWant"];
      std::string strr = aa + b;
      if (myredis.hashExist(str, js["nameWant"])) {
        myredis.hashDel(str, js["nameWant"]);
        myredis.hashDel(strr, js["myName"]);
        IO::SendMsg(fd, "Success", 8);
      } else {
        IO::SendMsg(fd, "用户不存在", 16);
      }
    } else if (status == 12) {  // 私聊
      std::string is_a = js["myName"];
      std::string is_b("_friend");
      std::string is = is_a + is_b;
      std::string wantj = js["nameWant"];
      int isfriend = myredis.hashExist(is, wantj);
      //std::cout << "flag: isfriend " << isfriend << std::endl;

      if (isfriend == 0) {// 判断是否为好友
        IO::SendMsg(fd, "not", 4);
      } else {
        std::string name_a = js["nameWant"];
        std::string name_b("_friend");
        std::string block = name_a + name_b;
        std::string flag = myredis.getHash(block, js["myName"]);
        if (strcmp(flag.c_str(), "FALSE") == 0) { // 判断是否屏蔽
          IO::SendMsg(fd, "BLOCK", 6);
        } else {
          std::string a = js["myName"];
          std::string b("_friend");
          std::string str = a + b;
          if (myredis.hashExist(str, js["nameWant"])) {
            std::cout << "让我们来处理聊天" << std::endl;
            if (userCtl.cycleFind(js["nameWant"])) {
              // 好友在线
              IO::SendMsg(fd, "online", 7);
              std::string onchat("onchat");
              std::string onchat_in("onchat_in");
              myredis.setHash(onchat, js["myName"], "true");
              myredis.saddValue(onchat_in, a); // 把自己加到聊天框
              int flag = myredis.sismember(onchat_in, js["nameWant"]);
              if(flag) { // 如果对方也进入到了聊天框
                IO::SendMsg(fd, "in", 3);
              }else {    // 对方没有进入聊天框
                IO::SendMsg(fd, "noin", 5);
              }
              
            } else {
              // 好友不在线
              std::cout << "buzai" << std::endl;
              IO::SendMsg(fd, "offline", 8);
            }
          } else {
            IO::SendMsg(fd, "not", 4);
          }
        }
      }
    } else if (status == 13) {  // offline留言
      std::string talk_a = js["name"];
      std::string talk_b(" talk to you: ");
      std::string talk_c = js["mess"];
      std::string talk = talk_a + talk_b + talk_c;

      std::string str_a = js["nameWant"];
      std::string str_b("_mess");
      std::string str = str_a + str_b;

      std::string str_d = js["nameWant"];
      std::string str_e("leave_ding");
      std::string ding = str_d + str_e;

      myredis.lpush(ding, js["name"]);
      myredis.lpush(str, talk);
    } else if (status == 14) {  // 接受留言
      std::string a = js["name"];
      std::string b("_mess");
      std::string str = a + b;

      int len = myredis.llen(str);
      std::string strr = std::to_string(len);

      IO::SendMsg(fd, strr.c_str(), strr.size());

      for (int i = 0; i < len; i++) {
        std::string temp = myredis.lpop(str);
        IO::SendMsg(fd, temp.c_str(), temp.size()+1);
      }
    } else if (status == 15) {  // 实时聊天
      std::string mes = js["mess"];
      std::string rec = js["nameWant"];
      int clifd = userCtl.getFd(rec);
      // std::cout << clifd << std::endl;
      // std::cout << mes << std::endl;
      if (strcmp(mes.c_str(), "exit") == 0) {
        if(flag_exit % 2 != 0) {
          std::string ms("成功退出聊天");
          std::string msg("对方已退出聊天");
          std::string onchat("onchat");
          std::string fa("false");
          myredis.setHash(onchat, js["name"], fa);
          std::string onchat_in("onchat_in");
          myredis.srmmember(onchat_in, js["name"]);
          int myfd = userCtl.getFd(js["name"]);
          IO::SendMsg(myfd, ms.c_str(), ms.size() + 1);
          IO::SendMsg(clifd, msg.c_str(), msg.size() + 1);
          flag_exit++;
        }else {
          std::string onchat("onchat");
          std::string fa("false");
          myredis.setHash(onchat, js["name"], fa);
          std::string onchat_in("onchat_in");
          myredis.srmmember(onchat_in, js["name"]);
          flag_exit++;
        }
      } else {
        std::string head = js["name"];
        std::string onchat("onchat");
        std::string flag = myredis.getHash(onchat, js["nameWant"]);
        std::string mid("给你发来了消息: ");
        std::string m = head + mid + mes;
        std::string history("hist_");
        std::string ab = history +head+rec;
        std::string ba = history +rec+head;
        std::string inhistory(" said: ");
        std::string in = head+ inhistory +mes;
        myredis.setHash(ab,in, "1");
        myredis.setHash(ba,in, "1");
        if(strcmp(flag.c_str(), "false")  == 0) {
          // pass
        }else {
          IO::SendMsg(clifd, m.c_str(), m.size()+1);
        }
      }
    } else if (status == 16) {  // 历史聊天记录
      std::string myname = js["myName"];
      std::string nameWant = js["nameWant"];
      std::string hist("hist_");
      std::string ab = hist+myname+nameWant;
      int len = myredis.hlen(ab);
      if(len > 0) { 
        std::vector<std::string> result;
        result = myredis.getHashKey(ab);
        std::string length = std::to_string(len);
        IO::SendMsg(fd, length.c_str(), length.size()+1);
        for(int i = 0; i < len; i++) {
          IO::SendMsg(fd, result[i].c_str(), result[i].size()+1);
        }
      } else {
        IO::SendMsg(fd, "notexist", 9);
      }
      

    } else if (status == 17) {  // 屏蔽
      std::string a = js["name"];
      std::string b("_friend");
      std::string str = a + b;
      if (myredis.hashExist(str, js["nameWant"])) {
        std::string mes = myredis.getHash(str, js["nameWant"]);
        if (strcmp(mes.c_str(), "FALSE") == 0) {
          IO::SendMsg(fd, "ALREADY", 8);
        } else {
          myredis.setHash(str, js["nameWant"], std::string("FALSE"));
          IO::SendMsg(fd, "YES", 4);
        }
      } else {
        IO::SendMsg(fd, "NOT", 4);
      }
    } else if (status == 18) {  // 解除屏蔽
      std::string a = js["name"];
      std::string b("_friend");
      std::string str = a + b;
      if (myredis.hashExist(str, js["nameWant"])) {
        std::string mes = myredis.getHash(str, js["nameWant"]);
        if (strcmp(mes.c_str(), "TRUE") == 0) {
          IO::SendMsg(fd, "ALREADY", 8);
        } else {
          myredis.setHash(str, js["nameWant"], std::string("TRUE"));
          IO::SendMsg(fd, "YES", 4);
        }
      } else {
        IO::SendMsg(fd, "NOT", 4);
      }
    } else if (status == 19) {  // 处理文件
      std::string is_a = js["name"];
      std::string is_b("_friend");
      std::string is = is_a + is_b;
      int isfriend = myredis.hashExist(is, js["nameWant"]);

      if (isfriend == 0) {
        IO::SendMsg(fd, "not", 4);
      } else {
        std::string name_a = js["nameWant"];
        std::string name_b("_friend");
        std::string block = name_a + name_b;
        std::string flag = myredis.getHash(block, js["name"]);
        if (strcmp(flag.c_str(), "FALSE") == 0) {
          IO::SendMsg(fd, "BLOCK", 6);
        } else {
          std::string a = js["name"];
          std::string b("_friend");
          std::string str = a + b;
          if (myredis.hashExist(str, js["nameWant"])) {
            std::cout << "让我们来处理文件" << std::endl;
            // if (userCtl.cycleFind(js["nameWant"])) {
            //  好友在线
            if (0) {
              std::string onchat("onchat");
              std::string flag_on("false");
              if (myredis.hashExist(onchat, js["nameWant"])) {
                flag_on = myredis.getHash(onchat, js["nameWant"]);
              }
              if (strcmp(flag_on.c_str(), "true") == 0) {
                IO::SendMsg(fd, "onchat", 7);
              } else {
                std::string tr("true");
                myredis.setHash(onchat, js["nameWant"], tr);
                IO::SendMsg(fd, "online", 7);
              }
            } else {
              std::string nameWant = js["nameWant"];
              std::string file_a("_files_list");
              std::string file_str = nameWant + file_a;

              std::string file_flag = myredis.getString(file_str);
              if (strcmp(file_flag.c_str(), "true") == 0) {
                IO::SendMsg(fd, "exist", 6);
              } else {
                std::cout << "buzai" << std::endl;
                IO::SendMsg(fd, "offline", 8);
              }
            }
          } else {
            IO::SendMsg(fd, "not", 4);
          }
        }
      }
    } else if (status == 20) {  // 将文件接受至服务器
      std::string filename = js["filepath"];
      std::string size_len = js["len"];
      __off_t size = atoll(size_len.c_str());
      std::string who = js["myName"];
      int clifd = userCtl.getFd(who);
      // int clifd = js["fd"];
      auto f = filename.rfind('/');
      filename.erase(0, f + 1);
      filename.insert(0, "../tempfiles/temp_");
      //
      fileflag = 1;
      std::cout << size << std::endl;
      //
      std::cout << "in 20, fileflag is: " << fileflag << std::endl;
      myfilename = filename;
      myclifd = clifd;
      mysize = size;
      std::cout << myfilename << std::endl;
      std::cout << myclifd << std::endl;
      std::cout << fileflag << std::endl;

      std::string nameWant = js["nameWant"];
      std::string file_a("_files_list");
      std::string file_b("_files_ding");
      std::string file_c("_files_name");
      std::string file_d("_files_send");
      std::string file_str = nameWant + file_a;
      std::string file_ding = nameWant + file_b;
      std::string file_name = nameWant + file_c;
      std::string file_send = nameWant + file_d;
      myredis.setString(file_str, "true");
      myredis.setString(file_ding, "true");
      myredis.setString(file_name, filename);
      myredis.setString(file_send, who);

    } else if (status == 21) {  // 将文件发送到对端
      std::string who = js["name"];
      int clifd = userCtl.getFd(who);
      std::string file_a("_files_list");
      std::string file_b("_files_send");
      std::string file_c("_files_name");
      std::string file_str = who + file_a;
      std::string file_who = who + file_b;
      std::string file_namefrom = who + file_c;
      std::string flag = myredis.getString(file_str);
      if (strcmp(flag.c_str(), "true") == 0) {
        std::string who_send = myredis.getString(file_who);
        IO::SendMsg(fd, who_send.c_str(), who_send.size());

        std::string file_name = myredis.getString(file_namefrom);
        std::string file_nameorigin = file_name;
        std::cout << file_name << std::endl;
        auto f = file_name.rfind('/');
        file_name.erase(0, f + 1);
        file_name.insert(0, "../client_recv/");
        std::cout << file_name << std::endl;
        IO::SendMsg(fd, file_name.c_str(), file_name.size() + 1);
        std::cout << file_name << std::endl;
        int filefd = open(file_nameorigin.c_str(), O_RDONLY);
        if (filefd < 0) {
          std::cout << "what the fuck" << std::endl;
        }
        __off_t size;
        struct stat file_stat;
        fstat(filefd, &file_stat);
        size = file_stat.st_size;
        std::string size_len = std::to_string(size);
        IO::SendMsg(fd, size_len.c_str(), size_len.size() + 1);

        while ((ret = sendfile(clifd, filefd, NULL, file_stat.st_size)) != 0) {
          if (ret == -1) {
          } else {
            std::cout << ret << std::endl;
          }
        }
        std::cout << "ret = " << ret << std::endl;

        myredis.setString(file_str, "false");
      } else {
        IO::SendMsg(fd, "notexist", 9);
      }
    } else if (status == 22) {  // 找回密码
      std::string name = js["name"];
      std::string key("userMap");
      int exist = myredis.hashExist(key, name);
      if(exist){
        IO::SendMsg(fd, "exist", 6);
        std::string value = myredis.getHash(key, name);
        json user = json::parse(value);
        std::string question = user["question"];
        IO::SendMsg(fd, question.c_str(), question.size()+1);

        char buffer[1024];
        IO::RecvMsg(fd, buffer, sizeof(buffer));
        std::string passwd(buffer);
        std::string pass = user["passwd"];
        if(strcmp(passwd.c_str(), pass.c_str()) == 0) {
          IO::SendMsg(fd, pass.c_str(), pass.size()+1);
        }else{
          IO::SendMsg(fd, "false", 6);
        }

      }else{
        IO::SendMsg(fd, "no", 3);
      }
      
    } else if (status == 23) {  // 注册群聊
      std::string group_tail("_G");
      std::string grouphave("_group");
      std::string groupname = js["nameWant"];
      std::string who = js["myName"];
      std::string name = groupname+group_tail;
      std::string have = who+grouphave;
      int group_flag = myredis.hlen(name);
      std::string groupMap("groupMap");
      if(group_flag != 0) {  // 群聊已注册 
        IO::SendMsg(fd, "failed", 7);
      }else {
        myredis.setHash(name, js["myName"], "master");
        myredis.saddValue(groupMap, groupname);
        myredis.saddValue(have, groupname);
        IO::SendMsg(fd, groupname.c_str(), groupname.size()+1);
      }
      char buffer[1024];
    } else if (status == 24) {  // 查看已加入群组
      std::string name = js["name"];
      std::string tail("_group");
      std::string have = name+ tail;
      int len = myredis.slen(have);
      std::string length = std::to_string(len);
      IO::SendMsg(fd, length.c_str(), length.size()+1);

      std::vector<std::string> ret;
      ret = myredis.smembers(have);
      for (int i = 0; i < len; i++) {
        std::string str = ret[i];
        IO::SendMsg(fd, str.c_str(), str.size());
      }

    } else if (status == 25) {  // 申请加入群聊
      std::string groupMap("groupMap");
      std::string gWant = js["nameWant"];
      std::string wanttail("_want");
      std::string already("_G");
      std::string who = js["myName"];
      std::string gApply = gWant + wanttail;
      std::string gAlready = gWant + already;
      int flag_already = myredis.hashExist(gAlready, who);
      if(flag_already) {
        IO::SendMsg(fd, "already", 8);
      } else {
        int flag_haveapply = myredis.hashExist(gApply, who);
        if (flag_haveapply == 1) {
          IO::SendMsg(fd, "repeat", 7);
        } else {
          int flag = myredis.sismember(groupMap, gWant);
          if (flag == 0) {
            IO::SendMsg(fd, "failed", 7);
          } else {
            //myredis.setHash(gApply, who, "1");
            myredis.lpush(gApply, who);
            IO::SendMsg(fd, "ok", 3);
          }
        }
      }
    } else if (status == 26) {  // 解散群聊
      /*
        myName为申请者名字
        namewant为想要申请的群聊的名字
      */
      std::string group("_group");
      std::string name = js["myName"];
      std::string usergroup = name+group;
      std::string groupWant = js["nameWant"];
      int isexist = myredis.sismember(usergroup, js["nameWant"]);
      std::cout << isexist << std::endl;
      if(!isexist) { // 判断是否有此群聊
        IO::SendMsg(fd, "nogroup", 8);
      }else {
        IO::SendMsg(fd, "yes", 4);
        std::string G("_G");
        std::string why = groupWant+G;
        std::string flag = myredis.getHash(why, name);
        if(strcmp(flag.c_str(), "master") != 0) {
          IO::SendMsg(fd, "nopower", 8);
        }else{
          IO::SendMsg(fd, "ok", 3);
          std::string groupmap("groupMap");
          myredis.srmmember(groupmap, groupWant);
          
          int len = myredis.hlen(why);
          std::vector<std::string> result;
          result = myredis.getHashKey(why);
          for(int i = 0; i < len; i++) {
            std::string gp = result[i]+group;
            myredis.srmmember(gp, groupWant);
            myredis.hashDel(why, result[i]);
          }

          std::string p("_Gper");
          std::string per = groupWant+p;
          int leng = myredis.hlen(per);
          std::vector<std::string> results;
          results = myredis.getHashKey(per);
          for(int i = 0; i < leng; i++) {
            myredis.hashDel(per, results[i]);
          }
        }

      }

    } else if (status == 27) {  // 处理群聊申请
      std::string myname = js["myName"];
      std::string groupname = js["nameWant"];
      std::string gp("_G");
      std::string group = groupname + gp;
      int isexist = myredis.hashExist(group, myname);
      if(!isexist) { //
        IO::SendMsg(fd, "not" , 4);
      }else {
        std::string flag = myredis.getHash(group, myname);
        if(strcmp(flag.c_str(), "master") == 0 || 
            strcmp(flag.c_str(), "manager") == 0 ) {
          IO::SendMsg(fd, "yes", 4);
          std::string want("_want");
          std::string apply = groupname+want;
          int len = myredis.llen(apply);
          if(len == 0) {
            IO::SendMsg(fd, "empty", 6);
          }else {
            std::string length = std::to_string(len);
            IO::SendMsg(fd, length.c_str(), length.size()+1);
            for(int i = 0; i < len; i++) {
              std::string people = myredis.lpop(apply);
              IO::SendMsg(fd, people.c_str(), people.size()+1);
            }
          }
        }else {
          IO::SendMsg(fd, "no", 3);
        }
      }

    } else if (status == 28) {  // 设置管理员
      std::string myname = js["myName"];
      std::string groupname = js["nameWant"];
      std::string gp("_G");
      std::string group = groupname + gp;
      std::string bedog = js["mess"];
      int isexist = myredis.hashExist(group, myname);
      if(!isexist) { //
        IO::SendMsg(fd, "not" , 4);
      }else {
        std::string flag = myredis.getHash(group, myname);
        if(strcmp(flag.c_str(), "master") == 0 ) {
          IO::SendMsg(fd, "yes", 4);
          if(myredis.hashExist(group, bedog)) {
          std::string flag = myredis.getHash(group, bedog);
          if(strcmp(flag.c_str(), "manager") == 0) {
            IO::SendMsg(fd, "already", 8);
          }else{
            IO::SendMsg(fd, "ss", 3);
            myredis.setHash(group, bedog, "manager");
          }
          }else{
            IO::SendMsg(fd, "notpeople", 9);
          }
        }else {
          IO::SendMsg(fd, "no", 3);
        }
      }
    } else if (status == 29) {  // 撤销管理员
      std::string myname = js["myName"];
      std::string groupname = js["nameWant"];
      std::string gp("_G");
      std::string group = groupname + gp;
      std::string bedog = js["mess"];
      int isexist = myredis.hashExist(group, myname);
      if(!isexist) { //
        IO::SendMsg(fd, "not" , 4);
      }else {
        std::string flag = myredis.getHash(group, myname);
        if (strcmp(flag.c_str(), "master") == 0) {
          IO::SendMsg(fd, "yes", 4);
          if (myredis.hashExist(group, bedog)) {
            std::string flag = myredis.getHash(group, bedog);
            if (strcmp(flag.c_str(), "normal") == 0) {
              IO::SendMsg(fd, "already", 8);
            } else {
              IO::SendMsg(fd, "ss", 3);
              myredis.setHash(group, bedog, "normal");
            }
          } else {
            IO::SendMsg(fd, "notpeople", 9);
          }
        } else {
          IO::SendMsg(fd, "no", 3);
        }
      }
    } else if (status == 30) {  // 禁言
      std::string myname = js["myName"];
      std::string groupname = js["nameWant"];
      std::string beban = js["mess"];
      std::string gp("_G");
      std::string gper("_Gper");
      std::string group = groupname + gp;
      std::string per = groupname + gper;

      int isexist = myredis.hashExist(group, myname);
      if (!isexist) {  //
        IO::SendMsg(fd, "not", 4);
      } else {
        IO::SendMsg(fd, "yes", 4);
        if (myredis.hashExist(group, beban)) {
          IO::SendMsg(fd, "exist", 8);
          std::string flag_beban = myredis.getHash(group, beban);
          if (strcmp(flag_beban.c_str(), "master") ==
              0) {  // 被禁言的人是群主的话
            IO::SendMsg(fd, "power", 6);
          } else {
            IO::SendMsg(fd, "ok", 3);
            std::string flag_mas = myredis.getHash(group, myname);
            if (strcmp(flag_mas.c_str(), "master") == 0) {  // 是群主的话
              IO::SendMsg(fd, "ok", 3);
              myredis.setHash(per, beban, "false");
            }
            if (strcmp(flag_mas.c_str(), "manager") == 0) {  // 是管理员的话
              if (strcmp(flag_beban.c_str(), "manager") == 0) {
                IO::SendMsg(fd, "friend", 7);
              } else {
                IO::SendMsg(fd, "ok", 3);
                myredis.setHash(per, beban, "false");
              }
            } else {  // 普通群员
              IO::SendMsg(fd, "low", 4);
            }
          }
        } else {
          IO::SendMsg(fd, "noexist", 6);
        }
      }
    } else if (status == 31) {  // 取消禁言
      std::string myname = js["myName"];
      std::string groupname = js["nameWant"];
      std::string beban = js["mess"];
      std::string gp("_G");
      std::string gper("_Gper");
      std::string group = groupname + gp;
      std::string per = groupname + gper;

      int isexist = myredis.hashExist(group, myname);
      if (!isexist) {  //
        IO::SendMsg(fd, "not", 4);
      } else {
        IO::SendMsg(fd, "yes", 4);
        if (myredis.hashExist(group, beban)) {
          IO::SendMsg(fd, "exist", 8);
          std::string flag_beban = myredis.getHash(group, beban);
          if (strcmp(flag_beban.c_str(), "master") ==
              0) {  // 被禁言的人是群主的话
            IO::SendMsg(fd, "power", 6);
          } else {
            IO::SendMsg(fd, "ok", 3);
            std::string flag_mas = myredis.getHash(group, myname);
            if (strcmp(flag_mas.c_str(), "master") == 0) {  // 是群主的话
              IO::SendMsg(fd, "ok", 3);
              myredis.setHash(per, beban, "true");
            } else if (strcmp(flag_mas.c_str(), "manager") ==
                       0) {  // 是管理员的话
              if (strcmp(flag_beban.c_str(), "manager") == 0) {
                IO::SendMsg(fd, "friend", 7);
              } else {
                IO::SendMsg(fd, "ok", 3);
                myredis.setHash(per, beban, "true");
              }
            } else {  // 普通群员
              IO::SendMsg(fd, "low", 4);
            }
          }
        } else {
          IO::SendMsg(fd, "noexist", 6);
        }
      }
    } else if (status == 32) {  // 开始群聊
      std::string myname = js["myName"];
      std::string nameWant = js["nameWant"];
      std::string on("_online");
      std::string in("_group");
      std::string ban("_Gper");
      std::string sta("_G");
      std::string online = nameWant+on;
      std::string ingroup = myname+in;
      std::string say = nameWant+ban;
      std::string mas = nameWant+sta;
      int flag = myredis.sismember(ingroup, nameWant);
      if(!flag) {
        IO::SendMsg(fd, "not", 4);
      }else {
        IO::SendMsg(fd, "yes", 4);
        std::string status = myredis.getHash(mas, myname);
        if(strcmp(status.c_str(), "master") == 0) {
          IO::SendMsg(fd, "notban", 7);
          myredis.saddValue(online, myname);
        } else {
          std::string ban = myredis.getHash(say, myname);
          if (strcmp(ban.c_str(), "false") == 0) {
            IO::SendMsg(fd, "beban", 6);
          } else {
            IO::SendMsg(fd, "notban", 7);
            myredis.saddValue(online, myname);  // 成功进入群聊
          }
        }
      }

    } else if (status == 33) {  // 添加群员
      std::string group = js["nameWant"];
      std::string man = js["myName"];
      std::string Gtail("_G");
      std::string Gper("_Gper");
      std::string GROUP = group+ Gtail;
      std::string permission = group+ Gper;
      std::string have("_group");
      std::string had = man+have;
      myredis.setHash(GROUP, man, "normal");
      myredis.setHash(permission, man, "true");
      myredis.saddValue(had, group);
    } else if (status == 34) {  // 踢出群聊
      std::string myname = js["myName"];
      std::string groupname = js["nameWant"];
      std::string beban = js["mess"];
      std::string gp("_G");
      std::string gper("_Gper");
      std::string group = groupname + gp;
      std::string per = groupname + gper;

      int isexist = myredis.hashExist(group, myname);
      if (!isexist) {  //
        IO::SendMsg(fd, "not", 4);
      } else {
        IO::SendMsg(fd, "yes", 4);
        if (myredis.hashExist(group, beban)) {
          IO::SendMsg(fd, "exist", 8);
          std::string flag_beban = myredis.getHash(group, beban);
          if (strcmp(flag_beban.c_str(), "master") ==
              0) {  // 被禁言的人是群主的话
            IO::SendMsg(fd, "power", 6);
          } else {
            IO::SendMsg(fd, "ok", 3);
            std::string flag_mas = myredis.getHash(group, myname);
            if (strcmp(flag_mas.c_str(), "master") == 0) {  // 是群主的话
              IO::SendMsg(fd, "ok", 3);

              std::string flag_g("_G");
              std::string flag_per("_Gper");
              std::string flag_own("_group");
              std::string flag_group = js["nameWant"];
              std::string flag_name = js["mess"];
              std::string group_G = flag_group+flag_g;
              std::string group_per = flag_per+flag_per;
              std::string name = flag_name+flag_own;
              myredis.hashDel(group_G, flag_name);
              myredis.hashDel(group_per, flag_name);
              myredis.hashDel(name, flag_group);

            }
            if (strcmp(flag_mas.c_str(), "manager") == 0) {  // 是管理员的话
              if (strcmp(flag_beban.c_str(), "manager") == 0) {
                IO::SendMsg(fd, "friend", 7);
              } else {
                IO::SendMsg(fd, "ok", 3);

                std::string flag_g("_G");
                std::string flag_per("_Gper");
                std::string flag_own("_group");
                std::string flag_group = js["nameWant"];
                std::string flag_name = js["mess"];
                std::string group_G = flag_group + flag_g;
                std::string group_per = flag_per + flag_per;
                std::string name = flag_name + flag_own;
                myredis.hashDel(group_G, flag_name);
                myredis.hashDel(group_per, flag_name);
                myredis.hashDel(name, flag_name);
              }
            } else {  // 普通群员
              IO::SendMsg(fd, "low", 4);
            }
          }
        } else {
          IO::SendMsg(fd, "noexist", 6);
        }
      }
    } else if (status == 35) {  // 查看成员
      std::string myname = js["myName"];
      std::string groupname = js["nameWant"];
      std::string gp("_G");
      std::string group = groupname + gp;
      std::string bedog = js["mess"];
      int isexist = myredis.hashExist(group, myname);
      std::cout << "myname: " << myname << std::endl;
      std::cout << "groupname: " << groupname << std::endl;
      std::cout << "isexist " << isexist << std::endl;
      if(!isexist) { //
        IO::SendMsg(fd, "not" , 4);
      } else {
        IO::SendMsg(fd, "yes", 4);
        int len = myredis.hlen(group);
        std::string length = std::to_string(len);
        std::cout << "length" << length << std::endl;
        IO::SendMsg(fd, length.c_str(), length.size()+1);
        std::vector<std::string> result;
        result = myredis.getHashKey(group);
        std::cout << "result0:" << result[0] << std::endl;
        sleep(0.5);
        for(int i = 0; i < len; i++) {
          sleep(0.5);
          IO::SendMsg(fd, result[i].c_str(), result[i].size()+1);
          std::cout << "done!  i / fd:" << i << " " << fd <<std::endl;
        }
        
      }
    } else if (status == 36) {  // 群聊
      std::string myname = js["myName"];
      std::string nameWant = js["nameWant"];
      std::string mess = js["mess"];
      std::string his("_Ghist");
      std::string on("_online");
      std::string online = nameWant+on;
      std::string history = nameWant+his;
      if(strcmp(mess.c_str(), "exit") == 0) {
        std::cout << "exit done" << std::endl; 
        myredis.srmmember(online, myname);
        int clifd = userCtl.getFd(myname);
        std::string ex("exit");
        IO::SendMsg(clifd, ex.c_str(), ex.size()+1);
      }else {
        std::vector<std::string> result;
        result = myredis.smembers(online);
        int len = myredis.slen(online);
        for(int i = 0; i < len; i++) {
          std::string who = result[i];
          if(who == myname) {
            //pass
          }else {
            int clifd = userCtl.getFd(result[i]);
            std::string whosaid("告诉大家: ");
            std::string chat = myname+whosaid+mess;
            myredis.setHash(history, chat, "1");
            IO::SendMsg(clifd, chat.c_str(), chat.size()+1);
          }
        }
      }
    } else if (status == 37) {  // 群聊天记录
      std::string myname = js["myName"];
      std::string groupname = js["nameWant"];
      std::string gp("_G");
      std::string hist("_Ghist");
      std::string group = groupname + gp;
      std::string history = groupname + hist;
      int isexist = myredis.hashExist(group, myname);
      if(!isexist) { //
        IO::SendMsg(fd, "not" , 4);
      } else {
        IO::SendMsg(fd, "yes", 4);
        int len = myredis.hlen(history);
        if (len == 0) {
          IO::SendMsg(fd, "empty", 6);
        } else {
          IO::SendMsg(fd, "notempty", 9);
          std::string length = std::to_string(len);
          IO::SendMsg(fd, length.c_str(), length.size() + 1);
          std::vector<std::string> result;
          result = myredis.getHashKey(history);
          for (int i = 0; i < len; i++) {
            IO::SendMsg(fd, result[i].c_str(),result[i].size()+1);
          }
        }
      }
    } else if (status == 38) {  // 对话框外聊天 
      std::string talk_a = js["name"];
      std::string talk_b(" talk to you: ");
      std::string talk_c = js["mess"];
      std::string talk = talk_a + talk_b + talk_c;

      std::string str_a = js["nameWant"];
      std::string str_b("_mess");
      std::string str = str_a + str_b;

      std::string str_d = js["nameWant"];
      std::string str_e("leave_ding");
      std::string ding = str_d + str_e;

      myredis.lpush(ding, js["name"]);
      myredis.lpush(str, talk);

      std::string onchat_in("onchat_in");

      int flag = myredis.sismember(onchat_in, js["nameWant"]); 

      std::cout << "!!!!!!!!!!!flag 1110: " << flag << std::endl;
      if(flag) {
        IO::SendMsg(fd, "havein", 7);
      }else {
        IO::SendMsg(fd, "notin", 6);
      }
    }

    else if (status == 100) {
      std::string str_a = js["name"];
      std::string str_b("leave_ding");
      std::string str_c("apply_ding");
      std::string str_d("_files_ding");
      std::string leave_ding = str_a + str_b;
      std::string apply_ding = str_a + str_c;
      std::string files_ding = str_a + str_d;
      // while(1){

      while(1) {  
        // 处理留言
        int leave_len = myredis.llen(leave_ding);
        if (leave_len != 0) {
          std::string leaves =  myredis.lpop(leave_ding);
          std::string lea("leave");
          std::string ll = lea+leaves;
          //std::cout << ll << std::endl;
          IO::SendMsg(fd, ll.c_str(), ll.size()+1);
        }

        // 处理私聊

        // 好友申请
        int apply_len = myredis.llen(apply_ding);
        if (apply_len != 0) {
          std::string mes_a("apply");
          std::string mes_b = myredis.lpop(apply_ding);
          std::string mes = mes_a + mes_b;
          std::cout << mes << std::endl;
          IO::SendMsg(fd, mes.c_str(), mes.size());
        }

        // 文件
        std::string files_flag = myredis.getString(files_ding);
        if (strcmp(files_flag.c_str(), "true") == 0) {
          std::string mes_c("file");
          myredis.setString(files_ding, "false");
          IO::SendMsg(fd, mes_c.c_str(), mes_c.size() + 1);
        }
      }
    }
  
  
  }
  free(info);
  return NULL;
}

#endif