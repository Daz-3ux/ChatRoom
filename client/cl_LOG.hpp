#ifndef CL_LOG_H
#define CL_LOG_H
#include <string.h>

#include <iostream>
#include <list>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <thread>
#include <pthread.h>

#include "../include/IO.hpp"
#include "../include/REDIS.hpp"
#include "../include/SOCK.hpp"
#include "cl_UI.hpp"
#include "cl_GROUP.hpp"

using json = nlohmann::json;

Sock SOCK;

bool isDegital(std::string str) {
  for (int i = 0; i < str.size(); i++) {
    if (str.at(i) == '-' && str.size() > 1)  // 有可能出现负数
      continue;
    if (str.at(i) > '9' || str.at(i) < '0') 
      return false;
  }
  return true;
}

enum Status {
  _REGISTER = 1,
  _LOGIN,
  _NEWPSWD,
  _DELONESELF,
  _RENAME,
  _OFF,
  _ADDFRIEND,
  _APPLYFRIEND,
  _MAPFRIEND,
  _LOOKFRIEND = 10,
  _DELFRIEND,
  _TALK,
  _OFFLINETALK,
  _RETALK,
  _ONLINECHAT,
  _HISTORY,
  _BLOCKFRIEND,
  _UNBLOCK,
  _FILE,
  _OFFLINEFILESEND = 20,
  _RECVFILE,
  _FINDPASSWD,
  _REGISTERGROUP,
  _JOINEDGROUP,
  _APPLYGROUP,
  _COMAPPLY
};

enum supplement {
  _OUTCHAT = 38
};

struct login_Info {
  std::string ip;
  int port;
  std::string name;
  int loginStatus;

  login_Info() = default;
};

// struct file_Info{
//   std::string ip;
//   int port;
//   int fd;
//   int fp;
//   off_t size;
//   std::string mess;
//   file_Info() = default;
// };

struct MsgData {
  std::string name;
  std::string passwd;
  int loginStatus;
  std::string question;
  std::string answer;

  MsgData() = default;
};

struct MsgInfo {
  std::string name;
  std::string passwd;
  int loginStatus;

  MsgInfo() = default;
};

struct MsgAdd {
  std::string nameWant;
  std::string myName;
  int loginStatus;

  MsgAdd() = default;
};

struct MsgEdit {
  std::string name;
  std::string oldPasswd;
  std::string newPasswd;
  int loginStatus;

  MsgEdit() = default;
};

struct MsgApply {
  std::string name;
  int loginStatus;

  MsgApply() = default;
};

struct MsgYes {
  std::string myName;
  std::string nameWant;
  int loginStatus;

  MsgYes() = default;
};

struct MsgTalk {
  std::string name;
  std::string nameWant;
  std::string mess;
  int loginStatus;

  MsgTalk() = default;
};

struct MsgFile {
  std::string myName;
  std::string nameWant;
  std::string len;
  std::string filepath;
  int loginStatus;

  MsgFile() = default;
};

std::string fileJson(MsgFile &msg) {
  json file;
  file["myName"] = msg.myName;
  file["nameWant"] = msg.nameWant;
  file["len"] = msg.len;
  file["filepath"] = msg.filepath;
  file["loginStatus"] = msg.loginStatus;

  std::string s = file.dump();
  return s;
};

std::string registerJson(MsgData &msg) {
  json root;
  root["name"] = msg.name;
  root["passwd"] = msg.passwd;
  root["loginStatus"] = msg.loginStatus;
  root["question"] = msg.question;
  root["answer"] = msg.answer;

  std::string s = root.dump();
  return s;
}

std::string loginJson(MsgInfo &msg) {
  json info;
  info["name"] = msg.name;
  info["passwd"] = msg.passwd;
  info["loginStatus"] = msg.loginStatus;

  std::string s = info.dump();
  return s;
}

std::string addJson(MsgAdd &msg) {
  json add;
  add["nameWant"] = msg.nameWant;
  add["myName"] = msg.myName;
  add["loginStatus"] = msg.loginStatus;

  std::string s = add.dump();
  return s;
}

std::string applyJson(MsgApply &msg) {
  json apply;
  apply["name"] = msg.name;
  apply["loginStatus"] = msg.loginStatus;

  std::string s = apply.dump();
  return s;
}

std::string editJson(MsgEdit &msg) {
  json edit;
  edit["name"] = msg.name;
  edit["oldPasswd"] = msg.oldPasswd;
  edit["newPasswd"] = msg.newPasswd;
  edit["loginStatus"] = msg.loginStatus;

  std::string s = edit.dump();
  return s;
}

std::string yesJson(MsgYes &msg) {
  json yes;
  yes["myName"] = msg.myName;
  yes["nameWant"] = msg.nameWant;
  yes["loginStatus"] = msg.loginStatus;

  std::string s = yes.dump();
  return s;
}

std::string talkJson(MsgTalk &msg) {
  json talk;
  talk["name"] = msg.name;
  talk["nameWant"] = msg.nameWant;
  talk["mess"] = msg.mess;
  talk["loginStatus"] = msg.loginStatus;

  std::string s = talk.dump();
  return s;
}

void registerNewAccount(MsgData &msg, int sockfd) {
  std::cout << "Register youself!" << std::endl;

  std::cout << "Please input a name(输入exit退出)" << std::endl;
  // std::cin >> msg.name;
  std::cin.get();
  getline(std::cin, msg.name);
  msg.name = removeSpaces(msg.name);

  if (strcmp(msg.name.c_str(), "exit") == 0) {
    std::cout << "包含非法字符,退出注册" << std::endl;
    return;
  }
  std::cout << "Please input a passwd" << std::endl;
  system("stty -echo");
  // std::cin >> msg.passwd;
  // std::cin.get();
  getline(std::cin, msg.passwd);
  msg.passwd = removeSpaces(msg.passwd);
  system("stty echo");

  std::cout << "Please input a secur-question" << std::endl;
  // std::cin >> msg.question;
  // std::cin.get();
  getline(std::cin, msg.question);
  msg.question = removeSpaces(msg.question);
  std::cout << "Please input a secur-answer" << std::endl;
  // std::cin >> msg.answer;
  //  std::cin.get();
  getline(std::cin, msg.answer);
  msg.answer = removeSpaces(msg.answer);

  msg.loginStatus = _REGISTER;

  std::string newone = registerJson(msg);
  // 发送新用户到服务器
  IO::SendMsg(sockfd, newone.c_str(), newone.size() + 1);

  char message[32];
  memset(message, 0, sizeof(message));
  IO::RecvMsg(sockfd, message, sizeof(message));
  std::string mess(message, sizeof(message));

  std::cout << mess << std::endl;
  firstMenu();
}

void updataPassword(int sockfd) {
  MsgEdit addmsg;

  std::cout << "请输入您的用户名" << std::endl;
  // std::cin >> addmsg.name;
  std::cin.get();
  getline(std::cin, addmsg.name);
  addmsg.name = removeSpaces(addmsg.name);
  std::cout << "请输入旧密码" << std::endl;

  system("stty -echo");
  // std::cin >> addmsg.oldPasswd;
  // std::cin.get();
  getline(std::cin, addmsg.oldPasswd);
  addmsg.oldPasswd = removeSpaces(addmsg.oldPasswd);
  system("stty echo");

  std::cout << "请输入新密码" << std::endl;
  system("stty -echo");
  // std::cin >> addmsg.newPasswd;
  // std::cin.get();
  getline(std::cin, addmsg.newPasswd);
  addmsg.newPasswd = removeSpaces(addmsg.newPasswd);
  system("stty echo");

  if (addmsg.oldPasswd == addmsg.newPasswd) {
    std::cout << "两次密码一致" << std::endl;
    return;
  }

  addmsg.loginStatus = 3;

  std::string mess = editJson(addmsg);

  IO::SendMsg(sockfd, mess.c_str(), mess.size() + 1);

  char message[32];
  memset(message, 0, sizeof(message));
  IO::RecvMsg(sockfd, message, sizeof(message));
  std::string ms(message, sizeof(message));
  int judge = strcmp("Success", ms.c_str());
  if (judge != 0) {
    if (strcmp("Faild_1", ms.c_str()) == 0) {
      std::cout << "用户不存在" << std::endl;
    } else {
      std::cout << "密码验证失败了" << std::endl;
    }
  } else {
    std::cout << "密码修改成功" << std::endl;
  }
}

void findPassword(int sockfd) {
  MsgApply find;
  std::cout << "请输入您的用户名" << std::endl;
  std::cin >> find.name;
  //find.name = inmsg.name;
  find.loginStatus = _FINDPASSWD;
  std::string find_info = applyJson(find);
  IO::SendMsg(sockfd, find_info.c_str(), find_info.size() + 1);

  char buffer[4096];
  IO::RecvMsg(sockfd, buffer, sizeof(buffer));
  std::string exist(buffer);
  if (strcmp(exist.c_str(), "exist") == 0) {
    IO::RecvMsg(sockfd, buffer, sizeof(buffer));
    std::string question(buffer);
    std::cout << "密保问题:" << question << std::endl;
    std::cout << "请输入答案" << std::endl;
    std::string passwd;
    std::cin >> passwd;
    IO::SendMsg(sockfd, passwd.c_str(), passwd.size() + 1);

    IO::RecvMsg(sockfd, buffer, sizeof(buffer));
    std::string flag(buffer);
    if(strcmp(flag.c_str(), "false") == 0) {
      std::cout << "密保验证失败" << std::endl;
    }else {
      std::cout << "您的密码为 " << flag << "(^▽^)" << std::endl;
    }

  }else{
    std::cout << "用户名错误" << std::endl;
  }
}

void delself(int sockfd) {
  MsgApply find;
  std::cout << "请输入您的用户名" << std::endl;
  std::cin >> find.name;
  //find.name = inmsg.name;
  find.loginStatus = _DELONESELF;
  std::string find_info = applyJson(find);
  IO::SendMsg(sockfd, find_info.c_str(), find_info.size() + 1);

  char buffer[1024];
  memset(buffer, 0, sizeof(buffer));
  IO::RecvMsg(sockfd, buffer, sizeof(buffer));
  std::string exist(buffer);
  if (strcmp(exist.c_str(), "exist") == 0) {
    std::string myPasswd;
    std::cout << "请输入密码" << std::endl;
    system("stty -echo");
    std::cin.get();
    getline(std::cin, myPasswd);
    myPasswd = removeSpaces(myPasswd);
    system("stty echo");
    IO::SendMsg(sockfd, myPasswd.c_str(), myPasswd.size()+1);



    IO::RecvMsg(sockfd, buffer, sizeof(buffer));
    std::string question(buffer);
    std::cout << "密保问题:" << question << std::endl;
    std::cout << "请输入答案" << std::endl;
    std::string passwd;
    std::cin >> passwd;
    IO::SendMsg(sockfd, passwd.c_str(), passwd.size() + 1);

    IO::RecvMsg(sockfd, buffer, sizeof(buffer));
    std::string flag(buffer);
    if(strcmp(flag.c_str(), "false") == 0) {
      std::cout << "验证失败" << std::endl;
    } else {
      memset(buffer, 0, sizeof(buffer));
      IO::RecvMsg(sockfd, buffer, sizeof(buffer));
      std::string can(buffer);
      if(strcmp(can.c_str(), "cant") == 0){
        std::cout << "您是某个群聊的群主,请解散群聊后再注销账号" << std::endl;
        return;
      } else{
        std::cout << "账号注销成功!" << std::endl;
      }
    }

  }else{
    std::cout << "用户名错误" << std::endl;
  }
}

void addFriend(MsgInfo &inmsg, int sockfd) {
  std::cout << "请输入你想加的好友的名称(输入exit退出,不会有人的名字叫exit的)"
            << std::endl;
  std::string nameWant;
  // std::cin >> nameWant;
  // std::cin.get();
  getline(std::cin, nameWant);
  nameWant = removeSpaces(nameWant);
  while (1) {
    if (strcmp(nameWant.c_str(), inmsg.name.c_str()) == 0) {
      std::cout << "你不能添加自己为好友,请重新输入" << std::endl;
      // std::cin >> nameWant;
      // std::cin.get();
      getline(std::cin, nameWant);
      nameWant = removeSpaces(nameWant);
    } else if (strcmp(nameWant.c_str(), "exit") == 0) {
      return;
    } else {
      break;
    }
  }
  MsgAdd msg;
  msg.myName = inmsg.name;
  msg.nameWant = nameWant;
  msg.loginStatus = _ADDFRIEND;
  std::string mess = addJson(msg);
  IO::SendMsg(sockfd, mess.c_str(), mess.size() + 1);

  char message[1024];
  memset(message, 0, sizeof(message));
  IO::RecvMsg(sockfd, message, sizeof(message));
  std::string ms(message, sizeof(message));
  std::cout << ms << std::endl;
  UserMenu2(inmsg.name);
}

void friendApply(MsgInfo &inmsg, int sockfd) {
  std::string a = inmsg.name;
  std::string b("_friApply");
  MsgApply app;
  app.name = a + b;
  app.loginStatus = _APPLYFRIEND;
  std::string mess = applyJson(app);
  IO::SendMsg(sockfd, mess.c_str(), mess.size() + 1);

  char message[1024];
  memset(message, 0, sizeof(message));
  IO::RecvMsg(sockfd, message, sizeof(message));
  std::string ms(message);
  // std::cout << ms << std::endl;
  if(strcmp(ms.c_str(), "empty") == 0) {
    std::cout << "暂无好友申请" << std::endl;
    return;
  }
  std::cout << "有以下人添加您为好友" << std::endl;
  int len = atoi(ms.c_str());
  for (int i = 0; i < len; i++) {
    memset(message, 0, sizeof(message));
    IO::RecvMsg(sockfd, message, sizeof(message));
    std::string ms(message);
    std::cout << "收到来自 " << ms << " 的好友请求" << std::endl;
    std::cout << "是否同意申请(y/N)" << std::endl;
    std::string judge;
    // std::cin >> judge;
    // std::cin.get();
    getline(std::cin, judge);
    judge = removeSpaces(judge);
    if (strcmp(judge.c_str(), "y") == 0) {
      MsgYes yes;
      yes.myName = inmsg.name;
      yes.nameWant = ms;
      yes.loginStatus = _MAPFRIEND;
      std::string mes = yesJson(yes);
      IO::SendMsg(sockfd, mes.c_str(), mes.size() + 1);
      std::cout << "添加成功!" << std::endl;
    } else {
      std::cout << "拒绝添加" << std::endl;
    }
  }
  UserMenu2(inmsg.name);
}

void friendMap(MsgInfo &inmsg, int sockfd) {
  MsgApply look;
  look.name = inmsg.name;
  look.loginStatus = _LOOKFRIEND;
  std::string mes = applyJson(look);
  IO::SendMsg(sockfd, mes.c_str(), mes.size() + 1);

  char mess[1024];
  memset(mess, 0, sizeof(mess));
  IO::RecvMsg(sockfd, mess, sizeof(mess));
  std::string ms(mess);
  int len = atoi(ms.c_str());

  // std::cout << "len453 : " << len << std::endl;
  
  for (int i = 0; i < len; i++) {
    // sleep(0.5);
    memset(mess, 0, sizeof(mess));
    IO::RecvMsg(sockfd, mess, sizeof(mess));
    std::string m(mess);
    std::cout << m << std::endl;
  }

  while (1) {
    secondFriendMenu();
    std::string opt;
    getline(std::cin, opt);
    opt = removeSpaces(opt);
    int option = atoi(opt.c_str());
    if (option == 1) {  // 屏蔽好友
      std::cout << "请输入要屏蔽的好友的名字" << std::endl;
      std::string name;
      getline(std::cin, name);
      name = removeSpaces(name);

      MsgTalk talk;
      talk.name = inmsg.name;
      talk.nameWant = name;
      talk.mess = std::string("block");
      talk.loginStatus = _BLOCKFRIEND;
      std::string s = talkJson(talk);
      IO::SendMsg(sockfd, s.c_str(), s.size() + 1);

      char message[1024];
      IO::RecvMsg(sockfd, message, sizeof(message));
      std::string ms(message);
      if (strcmp(ms.c_str(), "NOT") == 0) {
        std::cout << "对方还不是您的好友" << std::endl;
      } else if (strcmp(ms.c_str(), "ALREADY") == 0) {
        std::cout << "您已经屏蔽了对方" << std::endl;
      } else {
        std::cout << "屏蔽成功" << std::endl;
      }

    } else if (option == 2) {  // 解除屏蔽
      std::cout << "请输入解除屏蔽的好友的名字" << std::endl;
      std::string name;
      getline(std::cin, name);
      name = removeSpaces(name);

      MsgTalk talk;
      talk.name = inmsg.name;
      talk.nameWant = name;
      talk.mess = std::string("block");
      talk.loginStatus = _UNBLOCK;
      std::string s = talkJson(talk);
      IO::SendMsg(sockfd, s.c_str(), s.size() + 1);

      char message[1024];
      IO::RecvMsg(sockfd, message, sizeof(message));
      std::string ms(message);
      if (strcmp(ms.c_str(), "NOT") == 0) {
        std::cout << "对方还不是您的好友" << std::endl;
      } else if (strcmp(ms.c_str(), "ALREADY") == 0) {
        std::cout << "您并没有屏蔽对方" << std::endl;
      } else {
        std::cout << "解除屏蔽成功,继续愉快聊天吧!" << std::endl;
      }

    } else {
      UserMenu2(inmsg.name);
      break;
    }
  }
}

void delFriend(MsgInfo &inmsg, int sockfd) {
  MsgYes del;
  del.myName = inmsg.name;
  std::cout << "请输入想删除的好友" << std::endl;
  // std::cin >> del.nameWant;
  // std::cin.get();
  getline(std::cin, del.nameWant);
  del.nameWant = removeSpaces(del.nameWant);
  del.loginStatus = _DELFRIEND;

  std::string mess = yesJson(del);
  IO::SendMsg(sockfd, mess.c_str(), mess.size() + 1);

  char message[1024];
  IO::RecvMsg(sockfd, message, sizeof(message));
  std::string ms(message);
  std::cout << ms << std::endl;
  UserMenu2(inmsg.name);
}

void *reFromSe(void *arg) {
  int fd = *(int *)arg;
  char buffer[1024];
  while (1) {
    memset(buffer, 0, sizeof(buffer));
    int ret = IO::RecvMsg(fd, buffer, sizeof(buffer));
    std::string a(buffer);
    //std::cout << "a558: a & ret: " << a  << " & " << ret << std::endl;
    if (strcmp(a.c_str(), "对方已退出聊天") == 0) {
      std::cout << a << ",请输入exit退出" << std::endl;
      pthread_exit(0);
    } else if(strcmp(a.c_str(), "成功退出聊天") == 0) {
      pthread_exit(0);
    } else {
      //printf("\n");
      std::cout << '\n' << a << std::endl;
    }
  }

  return nullptr;
}

void talkto(MsgInfo &inmsg, int sockfd) {
  std::cout << "1.接收留言" << std::endl;
  std::cout << "2.开始聊天" << std::endl;
  std::cout << "3.查看历史聊天记录"  << std::endl;
  std::string opt;
  getline(std::cin, opt);
  opt = removeSpaces(opt);
if (strcmp(opt.c_str(), "1") == 0) {
    MsgApply rece;
    rece.name = inmsg.name;
    rece.loginStatus = _RETALK;
    std::string s = applyJson(rece);
    IO::SendMsg(sockfd, s.c_str(), s.size() + 1);

    char mess[1024];
    memset(mess, 0, sizeof(mess));
    IO::RecvMsg(sockfd, mess, sizeof(mess));
    std::string ms(mess);
    int len = atoi(ms.c_str());

    for (int i = 0; i < len; i++) {
      memset(mess, 0, sizeof(mess));
      IO::RecvMsg(sockfd, mess, sizeof(mess));
      std::string m(mess);
      std::cout << m << std::endl;
    }
} else if (strcmp(opt.c_str(), "2") == 0) {
  MsgYes talk;
  talk.myName = inmsg.name;
  std::cout << "请输入想要私聊的好友(输入exit退出)" << std::endl;
  getline(std::cin, talk.nameWant);
  talk.nameWant = removeSpaces(talk.nameWant);
  talk.loginStatus = _TALK;
  std::string mess = yesJson(talk);
  IO::SendMsg(sockfd, mess.c_str(), mess.size() + 1);

//again:
  char message[1024];
  memset(message, 0, sizeof(message));
  IO::RecvMsg(sockfd, message, sizeof(message));
  std::string ms(message);

  //std::cout << "ms616 : " << ms << std::endl;


  if (strcmp(ms.c_str(), "offline") == 0) {
    // 对方不在线
    std::cout << "对方不在线,请留言(输入exit退出)" << std::endl;
    MsgTalk talkPri;
    talkPri.name = inmsg.name;
    talkPri.nameWant = talk.nameWant;
    getline(std::cin, talkPri.mess);
    talkPri.mess = removeSpaces(talkPri.mess);
    talkPri.loginStatus = _OUTCHAT;

    if (strcmp(talkPri.mess.c_str(), "exit") == 0) {
      std::cout << "退出留言" << std::endl;
    } else {
      std::string mes = talkJson(talkPri);
      IO::SendMsg(sockfd, mes.c_str(), mes.size() + 1);
    }

  } else if (strcmp(ms.c_str(), "online") == 0) {
    //对方在线,实时聊天

    std::cout << "对方在线" << std::endl;

    MsgTalk talkIn;
    talkIn.name = inmsg.name;
    talkIn.nameWant = talk.nameWant;
    talkIn.loginStatus = _ONLINECHAT;

    MsgTalk talkOut;
    talkOut.name = inmsg.name;
    talkOut.nameWant = talk.nameWant;
    talkOut.loginStatus = _OUTCHAT;

    memset(message, 0, sizeof(message));
    IO::RecvMsg(sockfd, message, sizeof(message));
    std::string ms(message);
    if(strcmp(ms.c_str(), "in") == 0) {
in:
    std::cout << "开始实时聊天(输入exit退出)" << std::endl;
    pthread_t tid;
    pthread_create(&tid, NULL, reFromSe, (void *)&sockfd);
    pthread_detach(tid);

    while(true) {
      getline(std::cin, talkIn.mess);
      talkIn.mess = removeSpaces(talkIn.mess);
      std::string talkmsg = talkJson(talkIn);
      IO::SendMsg(sockfd, talkmsg.c_str(), talkmsg.size() + 1);
      if (strcmp("exit", talkIn.mess.c_str()) == 0) {
        sleep(0.5);
        break;
      }
    }
    // char to[1024];
    // memset(to, 0, sizeof(to));
    // IO::RecvMsg(sockfd, to, sizeof(to));
    
    }else if (strcmp(ms.c_str(), "noin") == 0){ // noin
      std::cout << "对方不在聊天菜单内,开始留言(输入exit退出)" << std::endl;
      while (true) {
        getline(std::cin, talkOut.mess);
        talkOut.mess = removeSpaces(talkOut.mess);
        if(strcmp(talkOut.mess.c_str(), "exit") == 0) {
          std::cout << "退出聊天"  << std::endl;
          break;
        }
        std::string ss = talkJson(talkOut);
        IO::SendMsg(sockfd, ss.c_str(), ss.size() + 1);

        char mes[1024];
        memset(mes, 0, sizeof(mes));
        IO::RecvMsg(sockfd, mes, sizeof(mes));
        std::string ms(mes);
        if(strcmp(ms.c_str(), "havein") == 0) {
          std::cout << "对方进入聊天界面,开始实时聊天" << std::endl;
          goto in;
        }else {
          std::cout << "继续留言(输入exit退出)" << std::endl;
        }
      }
    }




    
  } else if (strcmp(ms.c_str(), "BLOCK") == 0) {
    std::cout << "您被对方屏蔽了¯\'(⊙︿⊙)/¯" << std::endl;
  } else if (strcmp(ms.c_str(), "onchat") == 0) {
    std::cout << "对方已经在聊天了" << std::endl;
  } 
  // else if((ms.find("给你发来了消息")) != std::string::npos) {
  //   std::cout << ms << std::endl;
  //   goto again;
  // } 
  else if(strcmp(ms.c_str(), "not") == 0){
    std::cout << "对方不是你的好友" << std::endl;
  }
} else {  // opt == 3
  std::cout << "您想查看与谁的历史聊天记录(输入exit退出)" << std::endl;
  MsgYes his;
  getline(std::cin, his.nameWant);
  his.myName = inmsg.name;
  his.loginStatus = _HISTORY;
  std::string s = yesJson(his);
  IO::SendMsg(sockfd, s.c_str(), s.size() + 1);

  char message[1024];
  memset(message, 0, sizeof(message));
  IO::RecvMsg(sockfd, message, sizeof(message));
  std::string mess(message);
  if(strcmp(mess.c_str(), "notexist") == 0) {
    std::cout << "聊天记录为空" << std::endl;
  }else {
    int len = atoi(mess.c_str());
    for(int i = 0; i < len; i++) {
      memset(message, 0, sizeof(message));
      IO::RecvMsg(sockfd, message, sizeof(message));
      std::string mess(message);
      std::cout << mess << std::endl;
    }
  }

}
UserMenu();
}

// void *filesend(void *arg) {
//   file_Info *info = (file_Info *)arg;
//   std::string ip = info->ip;
//   int port = info->port;
//   int sockfd = Sock::Socket();
//   char *ipp = const_cast<char *>(ip.c_str());
//   Sock::Connect(sockfd, ipp, port);
//   int filefd = info->fp;
//   int size = info->size;
//   std::string mes_size = info->mess;
//   int ret;
//   IO::SendMsg(sockfd, mes_size.c_str(), mes_size.size() + 1);
//   while (1) {
//     ret = sendfile(sockfd, filefd, NULL, size);
//     if (ret == 0) {
//       printf("文件传送成功\n");
//       break;
//     }
//   }
//   pthread_exit(0);
// }

void filefunc(MsgInfo &inmsg, int sockfd) {
  std::cout << "1.接收文件" << std::endl;
  std::cout << "2.发送文件" << std::endl;
  std::string opt;
  getline(std::cin, opt);
  opt = removeSpaces(opt);
  if (strcmp(opt.c_str(), "1") == 0) {
    MsgApply recv;
    recv.name = inmsg.name;
    recv.loginStatus = _RECVFILE;
    std::string s = applyJson(recv);
    IO::SendMsg(sockfd, s.c_str(), s.size() + 1);

    char mess[4096];
    memset(mess, 0, sizeof(mess));
    IO::RecvMsg(sockfd, mess, sizeof(mess));
    std::string ms(mess);
    if (strcmp(ms.c_str(), "notexist") == 0) {
      std::cout << "还没有收到文件" << std::endl;
    } else {
      std::cout << "收到来自 " << ms << " 的文件" << std::endl;
      memset(mess, 0, sizeof(mess));
      IO::RecvMsg(sockfd, mess, sizeof(mess));
      std::string ms(mess);
      std::string where = ms;
      auto f = where.rfind('/');
      where.erase(0, f + 1);
      std::cout << "文件名为: " << where << std::endl;
      // std::cout << "文件将保存到为:" << ms << std::endl;
      IO::RecvMsg(sockfd, mess, sizeof(mess));
      std::string msg(mess);
      off_t size = atoll(msg.c_str());
      std::cout << "文件大小为: " << msg << std::endl;
      char buf[8192];
      int n = 0;
      // std::cout << ms << std::endl;
      FILE *fp = fopen(where.c_str(), "w");
      if (fp == nullptr) {
        std::cout << "errorrrrrrrrrrrrrrrr" << std::endl;
      }
      // std::cout << size << std::endl;
      while (size > 0) {
        if (sizeof(buf) < size) {
          n = IO::Readn(sockfd, buf, sizeof(buf));
        } else {
          n = IO::Readn(sockfd, buf, size);
        }
        if (n < 0) {
          continue;
        }
        // std::cout << n << std::endl;
        size -= n;
        fwrite(buf, n, 1, fp);
      }
      std::cout << "接收完毕" << std::endl;

      fclose(fp);
    }
  } else if(strcmp(opt.c_str(), "2") == 0){
    // 好友关系逻辑判断
    MsgTalk talk;
    talk.name = inmsg.name;
    std::cout << "想将文件发给谁?" << std::endl;
    getline(std::cin, talk.nameWant);
    talk.nameWant = removeSpaces(talk.nameWant);
    talk.loginStatus = _FILE;
    std::string mess = talkJson(talk);
    IO::SendMsg(sockfd, mess.c_str(), mess.size() + 1);

    char message[1024];
    memset(message, 0, sizeof(message));
    IO::RecvMsg(sockfd, message, sizeof(message));
    std::string ms(message);
    // std::cout << ms << std::endl;
    if (strcmp(ms.c_str(), "offline") == 0) {
      // 对方不在线
      std::cout << "对方将得到消息提醒" << std::endl;
      MsgFile file;
      file.myName = inmsg.name;
      file.nameWant = talk.nameWant;
      // std::cin.get();
      std::cout << "请输入您想要发送的文件及其路径名(输入exit退出)"
                << std::endl;
      getline(std::cin, file.filepath);
      file.filepath = removeSpaces(file.filepath);
      if (strcmp(file.filepath.c_str(), "exit") == 0) {
        std::cout << "成功退出" << std::endl;
        return;
      }
      int filefd = open(file.filepath.c_str(), O_RDONLY);
      if (filefd == -1) {
        std::cout << "Error In Open File" << std::endl;
        return;
      }
      file.loginStatus = _OFFLINEFILESEND;

      if (strcmp(file.filepath.c_str(), "exit") == 0) {
        std::cout << "退出文件发送" << std::endl;
      } else {
        __off_t size;
        struct stat file_stat;
        fstat(filefd, &file_stat);
        size = file_stat.st_size;
        std::string len = std::to_string(size);
        file.len = len;
        std::cout << "size :" << size << std::endl;
        // std::cout << "len :" << len << std::endl;

        std::string mes_size = fileJson(file);
        IO::SendMsg(sockfd, mes_size.c_str(), mes_size.size() + 1);

        // sleep(1);
        // filesent.fp = filefd;
        // filesent.size = size;
        // filesent.mess = mes_size;
        // pthread_t file;
        // pthread_create(&file, NULL, filesend, (void*)&filesent);
        // pthread_detach(file);
        int ret;
        while (1) {
          ret = sendfile(sockfd, filefd, NULL, size);
          // printf("---文件<%s>传送成功---\n", buf);
          //system("sync");
          if (ret == 0) {
            printf("文件传送成功\n");
            break;
          }
        }
        std::cout << "发送成功" << std::endl;
      }
    } else if (strcmp(ms.c_str(), "BLOCK") == 0) {
      std::cout << "您被对方屏蔽了¯\'(⊙︿⊙)/¯" << std::endl;
    } else if (strcmp(ms.c_str(), "exist") == 0) {
      std::cout << "对方已经收到一个文件" << std::endl;
    } else {  // ms.c_str() == not
      std::cout << "对方不是你的好友,留言失败" << std::endl;
    }
  }
  UserMenu2(inmsg.name);
}

// 群聊大家族
void registerGroup(MsgInfo &inmsg, int sockfd) {  // 注册群聊
  MsgYes group;
  group.myName = inmsg.name;
  std::cout << "请输入群名(输入exit退出)" << std::endl;
  getline(std::cin, group.nameWant);
  group.nameWant = removeSpaces(group.nameWant);
  if(strcmp(group.nameWant.c_str(), "exit") == 0) {
    std::cout << "退出注册" << std::endl;
    return;
    UserMenu2(inmsg.name);
  }
  group.loginStatus = _REGISTERGROUP;
  std::string group_info = yesJson(group);
  IO::SendMsg(sockfd, group_info.c_str(), group_info.size() + 1);

  char buffer[1024];
  IO::RecvMsg(sockfd, buffer, sizeof(buffer));
  std::string flag(buffer);
  if (strcmp(flag.c_str(), "failed") == 0) {
    std::cout << "群聊已存在" << std::endl;
  } else {
    std::cout << "您创建了名为: " << flag << " 的群聊, 您现在是群主"
              << std::endl;
  }
    UserMenu2(inmsg.name);
}

void allGroup(MsgInfo &inmsg, int sockfd) {  // 查看所有已加入群聊
  MsgApply joined;
  joined.name = inmsg.name;
  joined.loginStatus = _JOINEDGROUP;
  std::string str = applyJson(joined);
  IO::SendMsg(sockfd, str.c_str(), str.size() + 1);

  char mess[1024];
  IO::RecvMsg(sockfd, mess, sizeof(mess));
  std::string mes(mess);
  int len = atoi(mes.c_str());
  printf("\t群聊列表\t\n");
  printf("**********************\n");
  for (int i = 0; i < len; i++) {
    memset(mess, 0, sizeof(mess));
    IO::RecvMsg(sockfd, mess, sizeof(mess));
    std::string m(mess);
    std::cout << m << std::endl;
  }
  printf("**********************\n");

b:
  secondGroupMenu();
  while (1) {
    std::string action;
    getline(std::cin, action);
    if (std::cin.eof()) {
      std::cout << "害人不浅" << std::endl;
      return;
    }
    action = removeSpaces(action);
    if(!isDegital(action)) {
      std::cout << "您输入的: "<< action << "不是一个整数" << std::endl;
      goto b;
    }
    int opt = atoi(action.c_str());
    
    if(opt == 1 ) { // 解散群聊 
      deleteGroup(inmsg.name, sockfd);
      secondGroupMenu();
    }else if(opt == 2 ) { // 处理群申请
      groupApply(inmsg.name, sockfd);
      secondGroupMenu();
    }else if(opt == 3 ) { // 设置管理员
      setdog(inmsg.name, sockfd);
      secondGroupMenu();
    }else if(opt == 4 ) { // 撤销管理员
      notdog(inmsg.name, sockfd);
      secondGroupMenu();
    }else if(opt == 5 ) { // 禁言群成员
      ban(inmsg.name, sockfd);
      secondGroupMenu();
    }else if(opt == 6 ) { // 取消禁言
      noban(inmsg.name, sockfd);
      secondGroupMenu();
    }else if(opt == 7 ) { // 开始群聊
      joinchat(inmsg.name, sockfd);
      secondGroupMenu();
    }else if(opt == 8 ) { //踢出群聊
      deletesomeone(inmsg.name, sockfd);
      secondGroupMenu();
    }else if(opt == 9 ) { //查看群成员
      allpeople(inmsg.name, sockfd);
      secondGroupMenu();
    }else if(opt == 10 ) { // 查看群聊天记录
      allhistory(inmsg.name, sockfd);
      secondGroupMenu();
    }else if(opt == -1 ) { // 退出菜单
      std::cout << "即将退出群聊菜单" << std::endl;
      sleep(1);
      UserMenu();
      return;
    }else {
      std::cout << "输入错误,请重新输入" << std::endl;
      secondGroupMenu();
    }
  }
}

void joinGroup(MsgInfo &inmsg, int sockfd) {  // 申请加入群聊
  MsgAdd join;
  join.myName = inmsg.name;
  join.loginStatus = _APPLYGROUP;
  std::cout << "请输入你想要加入的群聊" << std::endl;
  getline(std::cin, join.nameWant);
  join.nameWant = removeSpaces(join.nameWant);
  std::string s = addJson(join);
  IO::SendMsg(sockfd, s.c_str(), s.size() + 1);

  char buf[1024];
  memset(buf, 0, 1024);
  IO::RecvMsg(sockfd, buf, sizeof(buf));
  std::string flag(buf);
  if (strcmp(flag.c_str(), "failed") == 0) {
    std::cout << "群聊不存在" << std::endl;
  } else if (strcmp(flag.c_str(), "repeat") == 0) {
    std::cout << "您已经申请过加入了" << std::endl;
  } else if (strcmp(flag.c_str(), "already") == 0) {
    std::cout << "您已经是群成员了" << std::endl;
  } else {
    std::cout << "成功提交申请" << std::endl;
  }
  UserMenu2(inmsg.name);
}

void *dd(void *arg) {
  login_Info *info = (login_Info *)arg;
  std::string ip = info->ip;
  int port = info->port;
  // std::cout << name << " " << ip << " " << port << " " << std::endl;
  int socket = Sock::Socket();
  char *ipp = const_cast<char *>(ip.c_str());
  Sock::Connect(socket, ipp, port);

  MsgApply dong;
  dong.name = info->name;
  dong.loginStatus = 100;
  std::string s = applyJson(dong);

  while (1) {
    // std::cout << "111111111111111" << std::endl;
    IO::SendMsg(socket, s.c_str(), s.size() + 1);

    char mess[1024];
    IO::RecvMsg(socket, mess, sizeof(mess));
    std::string mes(mess);
    // std::cout << mes << std::endl;
    if (strncmp("leave", mes.c_str(), 5) == 0) {
      std::cout << "mes:" << mes << std::endl;
      std::string who = mes.erase(0, 5);
      std::cout << "who:" << who << std::endl;
      std::cout << "你收到了一条留言,来自: "<< who << std::endl;
      std::cout << "如果对方在线,您可前往聊天界面与对方聊天" << std::endl;
    }
    if (strncmp("apply", mes.c_str(), 5) == 0) {
      std::string who = mes.erase(0, 5);
      std::cout << "您到了来自" << who << "的好友申请" << std::endl;
    }
    if (strcmp("file", mes.c_str()) == 0) {
      //std::cout << "您有一个文件待接收" << std::endl;
      std::cout << "有人正在向您传输文件" << std::endl;
    }
    if(strncmp("group", mes.c_str(), 5) == 0) {
      std::string someone = mes.erase(0, 5);
      std::cout << "您收到了来自 " << someone << " 的加群申请" << std::endl;
    }
    sleep(0.5);
    // std::cout << "2222222222222" << std::endl;
  }

  close(socket);
  pthread_exit(0);

  // return nullptr;
}


void loginAccount(MsgInfo &inmsg, login_Info &ding, int sockfd) {
  std::cout << "请输入您的昵称" << std::endl;
  // std::cin >> inmsg.name;
  std::cin.get();
  getline(std::cin, inmsg.name);
  inmsg.name = removeSpaces(inmsg.name);
  std::cout << "请输入您的密码" << std::endl;
  system("stty -echo");
  // std::cin >> inmsg.passwd;
  // std::cin.get();
  getline(std::cin, inmsg.passwd);
  inmsg.passwd = removeSpaces(inmsg.passwd);
  system("stty echo");

  inmsg.loginStatus = _LOGIN;

  std::string login_info = loginJson(inmsg);
  IO::SendMsg(sockfd, login_info.c_str(), login_info.size() + 1);

  char message[1024];
  memset(message, 0, sizeof(message));
  IO::RecvMsg(sockfd, message, sizeof(message));
  std::string mess(message);
  int judge = strcmp("Success", mess.c_str());

  if (strcmp("already", mess.c_str()) == 0) {
    std::cout << "该用户已经登陆" << std::endl;
    return;
  }

  if (judge != 0) {
    std::cout << "登陆失败" << std::endl;
  } else {
    UserMenu();

    ding.name = inmsg.name;
    pthread_t dingdong;
    pthread_create(&dingdong, NULL, dd, (void *)&ding);
    pthread_detach(dingdong);

a:
    while (1) {
      std::string action;
      std::cout << "请选择操作" << std::endl;
      // std::cin >> action;
      //std::cin.get();
      getline(std::cin, action);
      if (std::cin.eof()) {
        std::cout << "害人不浅" << std::endl;
        return;
      }
      action = removeSpaces(action);
      //std::cout<< "action:  " << action << std::endl;
      if(!isDegital(action)) {
        std::cout << "您输入的: " << action << "不是一个整数" << std::endl;
        UserMenu();
        goto a;
      }
      int opt = atoi(action.c_str());
      // std::cout << "opt: " << opt << std::endl;
      switch (opt) {
        case 1:  // 添加好友
        {
          addFriend(inmsg, sockfd);
          break;
        }
        case 2:  // 删除好友
        {
          delFriend(inmsg, sockfd);
          break;
        }
        case 3:  // 查看好友列表
        {
          friendMap(inmsg, sockfd);
          break;
        }
        case 4:  // 查看好友请求
        {
          friendApply(inmsg, sockfd);
          break;
        }
        case 5:  // 加入群
        {
          joinGroup(inmsg, sockfd);
          break;
        }
        case 6:  // 注册群
        {
          registerGroup(inmsg, sockfd);
          break;
        }
        case 7:  // 查看我加入的群列表
        {
          allGroup(inmsg, sockfd);
          break;
        }
        case 8:  // 私聊
        {
          talkto(inmsg, sockfd);
          break;
        }
        case 9:  // 管理文件
        {
          filefunc(inmsg, sockfd);
          break;
        }
        case -1:  // 退出
        {
          std::cout << "即将退出账号!" << std::endl;
          inmsg.loginStatus = _OFF;                
          std::string login_info = loginJson(inmsg);
          IO::SendMsg(sockfd, login_info.c_str(), login_info.size() + 1);
          sleep(1);
          firstMenu();
          return;
        }
        default: {
          std::cout << "请重新输入" << std::endl;
          UserMenu();
        }
      }
    }
  }
}

#endif