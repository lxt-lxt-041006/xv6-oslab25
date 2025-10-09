#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "user/user.h"
#include "kernel/fcntl.h"

// find <path> <name>
// 递归输出所有最终名称与 <name> 相同的文件或目录路径（含匹配的目录本身）。

static char *fmtname(char *path) {
  static char buf[DIRSIZ+1];
  char *p;
  for(p = path + strlen(path); p >= path && *p != '/'; p--) ;
  p++;
  int len = strlen(p);
  if(len >= DIRSIZ) return p;
  memmove(buf, p, len);
  buf[len] = 0;
  return buf;
}

static void find(char *path, char *target) {
  int fd;
  struct stat st;

  if((fd = open(path, 0)) < 0)
    return;
  if(fstat(fd, &st) < 0){
    close(fd);
    return;
  }

  if(st.type == T_FILE){
    if(strcmp(fmtname(path), target) == 0)
      printf("%s\n", path);
    close(fd);
    return;
  }

  if(st.type != T_DIR){
    close(fd);
    return;
  }

  // 目录：枚举其子项。
  char buf[512];
  struct dirent de;
  char *p;
  int n = strlen(path);
  if(n + 1 + DIRSIZ + 1 > sizeof(buf)){
    close(fd);
    return;
  }
  strcpy(buf, path);
  p = buf + n;
  if(n == 0 || buf[n-1] != '/') *p++ = '/';

  while(read(fd, &de, sizeof(de)) == sizeof(de)){
    if(de.inum == 0) continue;
    if(strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) continue;

    memmove(p, de.name, DIRSIZ); // 拷贝名字（含可能的 0 填充）
    p[DIRSIZ] = 0;               // 保证结尾 0

    // 去除填充后形成真实路径（memmove 已包含 0，fmtname 内部处理）
    // 判断是否名称匹配（文件或目录都打印）
    if(strcmp(fmtname(buf), target) == 0)
      printf("%s\n", buf);

    // 如果是目录则递归
    int cfd = open(buf, 0);
    if(cfd >= 0){
      struct stat cst;
      if(fstat(cfd, &cst) == 0 && cst.type == T_DIR){
        close(cfd);
        find(buf, target);
      } else {
        close(cfd);
      }
    }
  }
  close(fd);
}

int main(int argc, char *argv[]) {
  if(argc != 3){
    fprintf(2, "Usage: find <path> <name>\n");
    exit(1);
  }
  find(argv[1], argv[2]);
  exit(0);
}
