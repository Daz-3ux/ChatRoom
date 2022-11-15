#ifndef SE_IO_H
#define SE_IO_H

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <functional>
#include <iostream>
#include <string>

#include "../include/error.h"

class IO {
public:
  static ssize_t Read(int fd, void *ptr, size_t nbytes);
  static ssize_t Write(int fd, const void *ptr, size_t nbytes);
  static ssize_t Readn(int fd, void *vptr, size_t n);
  static ssize_t Writen(int fd, const void *vptr, size_t n);
  static ssize_t RecvMsg(int fd, void *vptr, size_t n);
  static ssize_t SendMsg(int fd, const void *vptr, size_t n);
};

ssize_t IO::Read(int fd, void *ptr, size_t nbytes) {
  ssize_t n;

again:
  if ((n = read(fd, ptr, nbytes)) == -1) {
    if (errno == EINTR || errno == EWOULDBLOCK)
      goto again;
    else
      return -1;
  }
  return n;
}
ssize_t IO::Write(int fd, const void *ptr, size_t nbytes) {
  ssize_t n;

again:
  if ((n = write(fd, ptr, nbytes)) == -1) {
    if (errno == EINTR)
      goto again;
    else
      return -1;
  }
  return n;
}

ssize_t IO::Readn(int fd, void *buffer, size_t n) {
  ssize_t numRead;
  size_t totRead;
  char *buf;

  buf = (char *)buffer;
  for (totRead = 0; totRead < n;) {
    numRead = read(fd, buf, n - totRead);

    if (numRead == 0) return totRead;
    if (numRead == -1) {
      if (errno == EINTR || errno == EWOULDBLOCK)
        continue;
      else
        return -1;
    }
    totRead += numRead;
    buf += numRead;
  }
  return totRead;
}

ssize_t IO::Writen(int fd, const void *buffer, size_t n) {
  ssize_t numWritten;
  size_t totWritten;
  const char *buf;

  buf = (char *)buffer;
  for (totWritten = 0; totWritten < n;) {
    numWritten = write(fd, buf, n - totWritten);

    if (numWritten <= 0) {
      if (numWritten == -1 && errno == EINTR)
        continue;
      else
        return -1;
    }
    totWritten += numWritten;
    buf += numWritten;
  }
  return totWritten;
}

inline void printStr(const char *s, int size) {
  // auto size = s.size();
  printf("length=%zu\n", size);
  uint32_t offset = 0;
  while (offset + 16 <= size) {
    const char *data = s + offset;
    fprintf(
        stderr,
        "%08x %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx "
        "%02hhx %02hhx %02hhx %02hhx "
        "%02hhx %02hhx %02hhx \t\t",
        offset, data[0], data[1], data[2], data[3], data[4], data[5], data[6],
        data[7], data[8], data[9], data[10], data[11], data[12], data[13],
        data[14], data[15]);
    for (int i = 0; i < 16; i++) {
      putc(data[i], stderr);
    }
    putc('\n', stderr);
    offset += 16;
  }
  if (offset < size) {
    const auto *data = s + offset;
    fprintf(stderr, "%08x ", offset);
    int i;
    for (i = 0; i < size - offset; i++) {
      fprintf(stderr, "%02hhx ", data[i]);
    }
    for (; i < 16; i++) {
      fprintf(stderr, "   ");
    }
    fprintf(stderr, "\t\t");
    for (int i = 0; i < size - offset; i++) {
      putc(data[i], stderr);
    }
    putc('\n', stderr);
  }
}

ssize_t IO::RecvMsg(int fd, void *vptr, size_t n)  // n为缓冲区的大小
{
  int ret;
  uint32_t len = 0;
  ret = IO::Readn(fd, (void *)&len, sizeof(len));
  // printStr((char*)&len, 4);
  if (ret == 0) {
    return 0;
  } else if (ret == -1) {
    return -1;
  }
  len = ntohl(len);
  if (len > n) {
    return -1;
  }
  ret = IO::Readn(fd, vptr, len);
  if (ret == 0) {
    return 0;
  } else if (ret == -1) {
    return -1;
  }
  // std::cout << "fd / size: " << fd << " " << len << std::endl;
  // printStr((char*)vptr, len);

  return ret;
}

ssize_t IO::SendMsg(int fd, const void *vptr, size_t n)  // n为要发送的字节大小
{
  int ret = 0;
  uint32_t len = n;
  len = htonl(len);
  Writen(fd, (void *)&len, sizeof(len));
  ret = Writen(fd, vptr, n);
  return ret;
}

std::string removeSpaces(const std::string &s) {
  std::string tmp(s);
  tmp.erase(std::remove(tmp.begin(), tmp.end(), ' '), tmp.end());
  return tmp;
}

void mygets(char *str, int num) {
  char *ret;
  int i = 0;
  ret = fgets(str, num, stdin);
  if (ret) {
    while (str[i] != '\n' && str[i] != '\0') {
      i++;
    }
    if (str[i] == '\n') {
      str[i] = '\0';
    } else {  // 丢弃过长字符
      while (getchar() != '\n') {
        continue;
      }
    }
  }
}

#endif