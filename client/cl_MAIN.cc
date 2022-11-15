#include "../include/SOCK.hpp"
#include "cl_LOG.hpp"
#include "cl_UI.hpp"

void loopAction(MsgData &msg, MsgInfo &inmsg, login_Info &ding, int sockfd) {
  // 增加程序健壮性
a:
  firstMenu();
  while (1) {
    std::string action;
    std::cout << "请选择操作" << std::endl;
    std::cin >> action;
    if (std::cin.eof()) {
      std::cout << "害人不浅" << std::endl;
      return;
    }
    if (!isDegital(action)) {
      std::cout << "您输入的: " << action << "不是一个整数" << std::endl;
      goto a;
    }
    int opt = atoi(action.c_str());
    switch (opt) {
      case 2:  // 注册
      {
        registerNewAccount(msg, sockfd);
        break;
      }
      case 1:  // 登陆
      {
        loginAccount(inmsg, ding, sockfd);
        break;
      }
      case 3:  // 修改密码
        updataPassword(sockfd);
        break;
      case 4:  // 找回密码
        findPassword(sockfd);
        break;
      case 5:  // 注销账号
        delself(sockfd);
        break;
      case -1:  // 退出
        // loginOff();
        std::cout << "程序即将退出!" << std::endl;
        close(sockfd);
        sleep(1);
        exit(EXIT_SUCCESS);
        break;
      default:
        std::cout << "错误输入,请重新输入" << std::endl;
        break;
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cout << "Usage: " << basename(argv[0]) << " ip_address port_number"
              << std::endl;
    std::cout << "程序即将退出, 下次请正确输入" << std::endl;
    sleep(0.5);
    exit(EXIT_FAILURE);
  } else {
    strncpy(IP, argv[1], sizeof(IP));
    PORT = atoi(argv[2]);
    std::cout << "ip_address: " << IP << " port_number: " << PORT << std::endl;
  }

  printf("pid: %d\n", getpid());

  int clientSocket = Sock::Socket();
  Sock::Connect(clientSocket, IP, PORT);

  std::cout << "服务器连接成功" << std::endl;
  MsgData msg;
  MsgInfo inmsg;
  login_Info ding;
  // file_Info filesent;
  ding.ip = IP;
  ding.port = PORT;
  // filesent.ip = IP;
  // filesent.port = PORT;
  loopAction(msg, inmsg, ding, clientSocket);
  close(clientSocket);

  return 0;
}