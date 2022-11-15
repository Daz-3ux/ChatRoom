#ifndef _CL_GROUP_H_
#define _CL_GROUP_H_

#include <nlohmann/json.hpp>

#include "../include/IO.hpp"
#include "../include/REDIS.hpp"
#include "../include/SOCK.hpp"

enum group {
  _DELGROUP = 26,
  _GROUPAPPLY,
  _SETDOG,
  _DELDOG,
  _BAN,
  _UNBAN,
  _STARTCHAT,
  _OKTOJOIN = 33,
  _DELETEMAN,
  _ALLPEOPLE,
  _CHAT,
  _CHATHISTORY
};

using json = nlohmann::json;

struct MsgCommon {
  std::string myName;
  std::string nameWant;
  std::string mess;
  int loginStatus;

  MsgCommon() = default;
};

std::string commonJson(MsgCommon &msg) {
  json common;
  common["nameWant"] = msg.nameWant;
  common["myName"] = msg.myName;
  common["mess"] = msg.mess;
  common["loginStatus"] = msg.loginStatus;

  std::string s = common.dump();
  return s;
}

void deleteGroup(std::string name, int sockfd) {  // 解散群聊
  MsgCommon del;
  del.myName = name;
  std::cout << "请输入您想要解散的群聊" << std::endl;
  getline(std::cin, del.nameWant);
  del.nameWant = removeSpaces(del.nameWant);
  del.mess = "";
  del.loginStatus = _DELGROUP;
  std::string s = commonJson(del);

  IO::SendMsg(sockfd, s.c_str(), s.size() + 1);

  char mess[1024];
  memset(mess, 0, sizeof(mess));
  IO::RecvMsg(sockfd, mess, sizeof(mess));
  std::string ss(mess);
  if (strcmp(ss.c_str(), "nogroup") == 0) {
    std::cout << "您想要解散的群聊不存在" << std::endl;
  } else {
    memset(mess, 0, sizeof(mess));
    IO::RecvMsg(sockfd, mess, sizeof(mess));
    std::string ss(mess);
    if (strcmp(ss.c_str(), "nopower") == 0) {
      std::cout << "只有群主可以解散群聊" << std::endl;
    } else {
      printf("*******************\n");
      std::cout << "解散成功!!!" << std::endl;
      printf("*******************\n");
    }
  }
}

void groupApply(std::string name, int sockfd) {  // 处理群聊申请
  std::cout << "请输入您想要管理的群" << std::endl;
  MsgCommon app;
  app.myName = name;
  getline(std::cin, app.nameWant);
  app.nameWant = removeSpaces(app.nameWant);
  app.mess = "1";
  app.loginStatus = _GROUPAPPLY;
  std::string s = commonJson(app);
  IO::SendMsg(sockfd, s.c_str(), s.size() + 1);

  char message[1024];
  memset(message, 0, sizeof(message));
  IO::RecvMsg(sockfd, message, sizeof(message));
  std::string mes(message);

  if (strcmp(mes.c_str(), "not") == 0) {
    std::cout << "您甚至不是该群的成员" << std::endl;
  } else if (strcmp(mes.c_str(), "no") == 0) {
    std::cout << "您的权限不够" << std::endl;
  } else if (strcmp(mes.c_str(), "yes") == 0) {
    memset(message, 0, sizeof(message));
    IO::RecvMsg(sockfd, message, sizeof(message));
    std::string mes(message);
    if (strcmp(mes.c_str(), "empty") == 0) {
      std::cout << "暂无申请" << std::endl;
    } else {
      int len = atoi(mes.c_str());
      for (int i = 0; i < len; i++) {
        memset(message, 0, sizeof(message));
        IO::RecvMsg(sockfd, message, sizeof(message));
        std::string mes(message);
        std::cout << "收到来自 " << mes << "的加群申请" << std::endl;
        std::cout << "是否同意申请(y/N)" << std::endl;

        std::string judge;
        getline(std::cin, judge);
        judge = removeSpaces(judge);
        if (strcmp(judge.c_str(), "y") == 0) {
          MsgCommon yes;
          yes.myName = mes;
          yes.nameWant = app.nameWant;
          yes.mess = " ";
          yes.loginStatus = _OKTOJOIN;
          std::string mes = commonJson(yes);
          IO::SendMsg(sockfd, mes.c_str(), mes.size() + 1);
          std::cout << "添加成功!" << std::endl;
        } else {
          std::cout << "拒绝成功" << std::endl;
        }
      }
    }
  }
}

void setdog(std::string name, int sockfd) {  // 设置管理员
  std::cout << "请输入您想要管理的群" << std::endl;
  MsgCommon app;
  app.myName = name;
  getline(std::cin, app.nameWant);
  app.nameWant = removeSpaces(app.nameWant);
  std::cout << "您想要将谁设置为管理员" << std::endl;
  getline(std::cin, app.mess);
  app.mess = removeSpaces(app.mess);
  if (app.myName == app.nameWant) {
    std::cout << "您不能设置自己为管理员" << std::endl;
    return;
  }
  app.loginStatus = _SETDOG;
  std::string s = commonJson(app);
  IO::SendMsg(sockfd, s.c_str(), s.size() + 1);

  char message[1024];
  memset(message, 0, sizeof(message));
  IO::RecvMsg(sockfd, message, sizeof(message));
  std::string mes(message);

  if (strcmp(mes.c_str(), "not") == 0) {
    std::cout << "您甚至不是该群的成员" << std::endl;
  } else if (strcmp(mes.c_str(), "no") == 0) {
    std::cout << "您的权限不够" << std::endl;
  } else if (strcmp(mes.c_str(), "yes") == 0) {
    memset(message, 0, sizeof(message));
    IO::RecvMsg(sockfd, message, sizeof(message));
    std::string mes(message);
    if (strcmp(mes.c_str(), "already") == 0) {
      std::cout << app.mess << "已经是管理员了" << std::endl;
    } else if (strcmp(mes.c_str(), "notpeople") == 0) {
      std::cout << "群聊中没有此人" << std::endl;
    } else {
      std::cout << app.mess << "成为了管理员!" << std::endl;
    }
  }
}

void notdog(std::string name, int sockfd) {  // 撤销管理员
  std::cout << "请输入您想要管理的群" << std::endl;
  MsgCommon app;
  app.myName = name;
  getline(std::cin, app.nameWant);
  app.nameWant = removeSpaces(app.nameWant);
  std::cout << "您想取消谁的管理员资格" << std::endl;
  getline(std::cin, app.mess);
  app.mess = removeSpaces(app.mess);
  if (app.myName == app.nameWant) {
    std::cout << "矛盾很你就" << std::endl;
    return;
  }
  app.loginStatus = _DELDOG;
  std::string s = commonJson(app);
  IO::SendMsg(sockfd, s.c_str(), s.size() + 1);

  char message[1024];
  memset(message, 0, sizeof(message));
  IO::RecvMsg(sockfd, message, sizeof(message));
  std::string mes(message);

  if (strcmp(mes.c_str(), "not") == 0) {
    std::cout << "您甚至不是该群的成员" << std::endl;
  } else if (strcmp(mes.c_str(), "no") == 0) {
    std::cout << "您的权限不够" << std::endl;
  } else if (strcmp(mes.c_str(), "yes") == 0) {
    memset(message, 0, sizeof(message));
    IO::RecvMsg(sockfd, message, sizeof(message));
    std::string mes(message);
    if (strcmp(mes.c_str(), "already") == 0) {
      std::cout << app.mess << "本来就不是管理员" << std::endl;
    } else if (strcmp(mes.c_str(), "notpeople") == 0) {
      std::cout << "群聊中没有此人" << std::endl;
    } else {
      std::cout << app.mess << "不再是管理员了!" << std::endl;
    }
  }
}

void ban(std::string name, int sockfd) {  // 禁言群成员
  MsgCommon ban;
  ban.myName = name;
  std::cout << "请输入您想要管理的群聊" << std::endl;
  getline(std::cin, ban.nameWant);
  ban.nameWant = removeSpaces(ban.nameWant);
  std::cout << "请输入您想要禁言的人" << std::endl;
  getline(std::cin, ban.mess);
  ban.mess = removeSpaces(ban.mess);
  ban.loginStatus = _BAN;
  std::string s = commonJson(ban);
  IO::SendMsg(sockfd, s.c_str(), s.size() + 1);

  char buf[1024];
  memset(buf, 0, sizeof(buf));
  IO::RecvMsg(sockfd, buf, sizeof(buf));
  std::string mes(buf);

  if (strcmp(mes.c_str(), "not") == 0) {
    std::cout << "您甚至不是该群的成员" << std::endl;
  } else if (strcmp(mes.c_str(), "yes") == 0) {
    memset(buf, 0, sizeof(buf));
    IO::RecvMsg(sockfd, buf, sizeof(buf));
    std::string ms(buf);
    if (strcmp(ms.c_str(), "exist") == 0) {
      memset(buf, 0, sizeof(buf));
      IO::RecvMsg(sockfd, buf, sizeof(buf));
      std::string ms(buf);
      if (strcmp(ms.c_str(), "power") == 0) {
        std::cout << "为什么禁言群主, 要造反吗" << std::endl;
      } else {
        memset(buf, 0, sizeof(buf));
        IO::RecvMsg(sockfd, buf, sizeof(buf));
        std::string msg(buf);
        if (strcmp(msg.c_str(), "friend") == 0) {
          std::cout << "对方也是管理员,相煎何太急" << std::endl;
        } else if (strcmp(msg.c_str(), "low") == 0) {
          std::cout << "权限不够" << std::endl;
        } else {
          std::cout << "禁言成功" << std::endl;
        }
      }
    } else {
      std::cout << "对方不是群成员" << std::endl;
    }
  }
}

void noban(std::string name, int sockfd) {  // 取消禁言
  MsgCommon ban;
  ban.myName = name;
  std::cout << "请输入您想要管理的群聊" << std::endl;
  getline(std::cin, ban.nameWant);
  ban.nameWant = removeSpaces(ban.nameWant);
  std::cout << "请输入您想要取消禁言的人" << std::endl;
  getline(std::cin, ban.mess);
  ban.mess = removeSpaces(ban.mess);
  ban.loginStatus = _UNBAN;
  std::string s = commonJson(ban);
  IO::SendMsg(sockfd, s.c_str(), s.size() + 1);

  char buf[1024];
  memset(buf, 0, sizeof(buf));
  IO::RecvMsg(sockfd, buf, sizeof(buf));
  std::string mes(buf);

  if (strcmp(mes.c_str(), "not") == 0) {
    std::cout << "您甚至不是该群的成员" << std::endl;
  } else if (strcmp(mes.c_str(), "yes") == 0) {
    memset(buf, 0, sizeof(buf));
    IO::RecvMsg(sockfd, buf, sizeof(buf));
    std::string ms(buf);
    if (strcmp(ms.c_str(), "exist") == 0) {
      memset(buf, 0, sizeof(buf));
      IO::RecvMsg(sockfd, buf, sizeof(buf));
      std::string m(buf);
      if (strcmp(m.c_str(), "power") == 0) {
        std::cout << "群主是不会被捂嘴滴" << std::endl;
      } else {
        memset(buf, 0, sizeof(buf));
        IO::RecvMsg(sockfd, buf, sizeof(buf));
        std::string msg(buf);
        if (strcmp(msg.c_str(), "friend") == 0) {
          std::cout << "对方也是管理员,你帮不了他的" << std::endl;
        } else if (strcmp(msg.c_str(), "low") == 0) {
          std::cout << "权限不够" << std::endl;
        } else {
          std::cout << "取消禁言成功" << std::endl;
        }
      }
    } else {
      std::cout << "对方不是群成员" << std::endl;
    }
  }
}

void *reFromChat(void *arg) {
  int fd = *(int *)arg;
  char buffer[1024];
  while (1) {
    memset(buffer, 0, sizeof(buffer));
    int len = IO::RecvMsg(fd, buffer, sizeof(buffer));
    // printf("%d :receive len in 319", len);
    std::string a(buffer);
    if (strcmp(a.c_str(), "exit") == 0) {
      pthread_exit(0);
    } else {
      std::cout << a << std::endl;
    }
  }

  return nullptr;
}

void joinchat(std::string name, int sockfd) {  // 开始群聊
  std::cout << "请输入您想要聊天的群聊" << std::endl;
  MsgCommon chat;
  chat.myName = name;
  getline(std::cin, chat.nameWant);
  chat.nameWant = removeSpaces(chat.nameWant);
  chat.loginStatus = _STARTCHAT;
  chat.mess = " ";
  std::string s = commonJson(chat);
  IO::SendMsg(sockfd, s.c_str(), s.size() + 1);
  char message[1024];
  memset(message, 0, sizeof(message));
  IO::RecvMsg(sockfd, message, sizeof(message));
  std::string mes(message);
  if (strcmp(mes.c_str(), "not") == 0) {
    std::cout << "您不是该群聊成员" << std::endl;
    return;
  } else {
    memset(message, 0, sizeof(message));
    IO::RecvMsg(sockfd, message, sizeof(message));
    std::string mes(message);
    if (strcmp(mes.c_str(), "beban") == 0) {
      std::cout << "您已被禁言" << std::endl;
    } else {
      std::cout << "成功进入群聊(输入exit退出)" << std::endl;

      pthread_t tid;
      pthread_create(&tid, NULL, reFromChat, (void *)&sockfd);
      pthread_detach(tid);

      MsgCommon talk;
      talk.myName = name;
      talk.nameWant = chat.nameWant;
      talk.loginStatus = _CHAT;

      while (true) {
        getline(std::cin, talk.mess);
        talk.mess = removeSpaces(talk.mess);
        std::string com(talk.mess);

        std::string mes = commonJson(talk);
        IO::SendMsg(sockfd, mes.c_str(), mes.size() + 1);
        if (strcmp("exit", com.c_str()) == 0) {
          break;
        }
      }
    }
  }
  std::cout << "退出聊天" << std::endl;
}

void deletesomeone(std::string name, int sockfd) {  //踢出群聊
  MsgCommon del;
  del.myName = name;
  std::cout << "请输入您想要管理的群聊" << std::endl;
  getline(std::cin, del.nameWant);
  del.nameWant = removeSpaces(del.nameWant);
  std::cout << "请输入您想要踢出的人" << std::endl;
  getline(std::cin, del.mess);
  del.mess = removeSpaces(del.mess);
  del.loginStatus = _DELETEMAN;
  std::string s = commonJson(del);
  IO::SendMsg(sockfd, s.c_str(), s.size() + 1);

  char buf[1024];
  memset(buf, 0, sizeof(buf));
  IO::RecvMsg(sockfd, buf, sizeof(buf));
  std::string mes(buf);

  if (strcmp(mes.c_str(), "not") == 0) {
    std::cout << "您甚至不是该群的成员" << std::endl;
  } else if (strcmp(mes.c_str(), "yes") == 0) {
    memset(buf, 0, sizeof(buf));
    IO::RecvMsg(sockfd, buf, sizeof(buf));
    std::string ms(buf);
    if (strcmp(ms.c_str(), "exist") == 0) {
      memset(buf, 0, sizeof(buf));
      IO::RecvMsg(sockfd, buf, sizeof(buf));
      std::string ms(buf);
      if (strcmp(ms.c_str(), "power") == 0) {
        std::cout << "为什么踢出群主, 要造反吗" << std::endl;
      } else {
        memset(buf, 0, sizeof(buf));
        IO::RecvMsg(sockfd, buf, sizeof(buf));
        std::string msg(buf);
        if (strcmp(msg.c_str(), "friend") == 0) {
          std::cout << "对方也是管理员,相煎何太急" << std::endl;
        } else if (strcmp(msg.c_str(), "low") == 0) {
          std::cout << "权限不够" << std::endl;
        } else if (strcmp(msg.c_str(), "ok") == 0) {
          std::cout << "踢出成功" << std::endl;
        }
      }
    } else {
      std::cout << "对方不是群成员" << std::endl;
    }
  }
}

void allpeople(std::string name, int sockfd) {  // 查看群聊中所有成员
  MsgCommon all;
  all.myName = name;
  std::cout << "请输入您想查看的群聊" << std::endl;
  getline(std::cin, all.nameWant);
  all.nameWant = removeSpaces(all.nameWant);
  all.mess = " ";
  all.loginStatus = _ALLPEOPLE;
  std::string s = commonJson(all);

  IO::SendMsg(sockfd, s.c_str(), s.size() + 1);
  char buf[1024];
  memset(buf, 0, sizeof(buf));
  IO::RecvMsg(sockfd, buf, sizeof(buf));
  std::string mes(buf);

  // std::cout << "mes445: "<< mes << std::endl;

  if (strcmp(mes.c_str(), "not") == 0) {
    std::cout << "您甚至不是该群的成员" << std::endl;
  } else if (strcmp(mes.c_str(), "yes") == 0) {
    memset(buf, 0, sizeof(buf));
    IO::RecvMsg(sockfd, buf, sizeof(buf));
    std::string mes(buf);
    // std::cout << "mes453: " << mes << std::endl;
    int len = atoi(mes.c_str());
    // std::cout << "len455: " << len << std::endl;
    std::cout << "成员列表如下" << std::endl;
    printf("*******************\n");

    for (int i = 0; i < len; i++) {
      memset(buf, 0, sizeof(buf));
      // std::cout << "recv fd: " << sockfd << std::endl;
      IO::RecvMsg(sockfd, buf, sizeof(buf));
      std::string mes(buf);
      std::cout << mes << std::endl;
    }
    printf("*******************\n");
  } else {
    std::cout << "111in468 " << mes << std::endl;
  }
}

void allhistory(std::string name, int sockfd) {  // 查看群聊聊天记录
  MsgCommon all;
  all.myName = name;
  std::cout << "请输入您想查看的群聊" << std::endl;
  getline(std::cin, all.nameWant);
  all.nameWant = removeSpaces(all.nameWant);
  all.mess = " ";
  all.loginStatus = _CHATHISTORY;
  std::string s = commonJson(all);

  IO::SendMsg(sockfd, s.c_str(), s.size() + 1);
  char buf[1024];
  memset(buf, 0, sizeof(buf));
  IO::RecvMsg(sockfd, buf, sizeof(buf));
  std::string mes(buf);

  if (strcmp(mes.c_str(), "not") == 0) {
    std::cout << "您甚至不是该群的成员" << std::endl;
  } else if (strcmp(mes.c_str(), "yes") == 0) {
    memset(buf, 0, sizeof(buf));
    IO::RecvMsg(sockfd, buf, sizeof(buf));
    std::string mes(buf);
    if (strcmp(mes.c_str(), "empty") == 0) {
      std::cout << "聊天记录为空" << std::endl;
    } else {
      memset(buf, 0, sizeof(buf));
      IO::RecvMsg(sockfd, buf, sizeof(buf));
      std::string mes(buf);
      int len = atoi(mes.c_str());
      std::cout << "聊天记录如下" << std::endl;
      printf("*******************\n");
      for (int i = 0; i < len; i++) {
        memset(buf, 0, sizeof(buf));
        IO::RecvMsg(sockfd, buf, sizeof(buf));
        std::string mes(buf);
        std::cout << mes << std::endl;
      }
      printf("*******************\n");
    }
  }
}
#endif