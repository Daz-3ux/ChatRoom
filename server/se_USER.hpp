#ifndef _SE_USER_H
#define _SE_USER_H

#include <list>
#include <string>



struct MsgUserFd
{
  std::string name;
  int fd;

  MsgUserFd() = default;
};

MsgUserFd msgFd;

class User
{
public:
  User() {
    
  };

  void addInList(std::string name, int fd)
  {
    msgFd.name = name;
    msgFd.fd = fd;
    onlineList.emplace_back(msgFd);
  }

  bool cycleFind(std::string name)
  {
    for(auto it = onlineList.begin(); it != onlineList.end(); it++){
      if(it->name == name){
        return true;
      }
    }
    return false;
  }

  void cycle()
  {
    for(auto it = onlineList.begin(); it != onlineList.end(); it++){
      std::cout << it->name << " is " << it->fd << std::endl;
    }
  }


  void delInList(std::string name)
  {
    auto it = onlineList.begin();
    for(; it != onlineList.end(); ) {
      if(it->name == name){
        it = onlineList.erase(it);
      }
      it++;
    }
  }

  void delInList2(int fd)
  {
    auto it = onlineList.begin();
    for( ; it != onlineList.end(); ) {
      if(it->fd == fd){
        it = onlineList.erase(it);
      }
    }
    it++;
  }

  int getFd(std::string name)
  {
    auto it = onlineList.begin();
    while (it != onlineList.end()) {
      if(it->name == name) {
        return it->fd;
      }
      it++;
    }
    return -1;
  }

private:
  std::list<MsgUserFd> onlineList;
};


#endif