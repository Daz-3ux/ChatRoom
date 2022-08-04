#ifndef CL_UI_H
#define CL_UI_H

#include <iostream>

void firstMenu() {
  printf("\n");
  printf("\t       DAZ---聊天室\t\n");
  printf("/*******************************************/\n");
  printf("/*  ____             ____ _           _    */\n");
  printf("/* |  _ \'  __ _ ____/ ___| |__   __ _| |_  */\n");
  printf("/* | | | |/ _` |_  / |   | '_ \' / _` | __| */\n");
  printf("/* | |_| | (_| |/ /| |___| | | | (_| | |_  */\n");
  printf("/* |____/ \'__,_/___|\'____|_| |_|\'__,_|\'__| */\n");
  printf("/*                                         */\n");
  printf("/*******************************************/\n");
  printf("\t--------------------------------\n");
  printf("\t1.注册\n");
  printf("\t2.登入\n");
  printf("\t3.修改密码\n");
  // printf("\t4.修改昵称\n");
  printf("\t4.找回密码\n");
  printf("\t-1.退出\n");
  printf("\t--------------------------------\n");
}

void UserMenu() {
  printf("\n");
  printf("\t       欢迎进入聊天室\t\n");
  printf("/*******************************************/\n");
  printf("/*  ____             ____ _           _    */\n");
  printf("/* |  _ \'  __ _ ____/ ___| |__   __ _| |_  */\n");
  printf("/* | | | |/ _` |_  / |   | '_ \' / _` | __| */\n");
  printf("/* | |_| | (_| |/ /| |___| | | | (_| | |_  */\n");
  printf("/* |____/ \'__,_/___|\'____|_| |_|\'__,_|\'__| */\n");
  printf("/*                                         */\n");
  printf("/*******************************************/\n");
  printf("\t--------------------------------\n");
  printf("\t1.添加好友\n");
  printf("\t2.删除好友\n");
  printf("\t3.查看好友列表\n");
  printf("\t4.查看好友请求\n");
  printf("\t5.加入群\n");
  printf("\t6.注册群\n");
  printf("\t7.查看已加入群列表\n");
  printf("\t8.私聊\n");
  printf("\t9.管理文件\n");
  printf("\t-1.退出登陆\n");
  printf("\t--------------------------------\n");
}

void secondFriendMenu() {
  std::cout << "1.屏蔽好友消息" << std::endl;
  std::cout << "2.解除好友屏蔽" << std::endl;
  std::cout << "3.退出好友列表" << std::endl;
}

void secondGroupMenu() {
  std::cout << "1.解散群聊" << std::endl;
  std::cout << "2.处理群申请" << std::endl;
  std::cout << "3.设置管理员" << std::endl;
  std::cout << "4.撤销管理员" << std::endl;
  std::cout << "5.禁言群成员" << std::endl;
  std::cout << "6.取消禁言" << std::endl;
  std::cout << "7.开始水群" << std::endl;
  std::cout << "8.踢出群聊" << std::endl;
  std::cout << "9.查看群成员" << std::endl;
  std::cout << "10.查看群聊聊天记录" << std::endl;
  std::cout << "-1.退出菜单" << std::endl;
}
#endif