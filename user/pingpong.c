#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// pingpong 实现：
// 父进程 -> 子进程 发送父进程 pid (int)
// 子进程 打印 "<child>: received ping from pid <parent>"
// 子进程 -> 父进程 回传子进程 pid (int)
// 父进程 打印 "<parent>: received pong from pid <child>"
// 不使用 wait()，仅依赖管道阻塞。

static int read_full(int fd, void *buf, int n) {
  int tot = 0; char *p = (char*)buf; int r;
  while (tot < n && (r = read(fd, p + tot, n - tot)) > 0) tot += r;
  return tot == n ? 0 : -1;
}
static int write_full(int fd, const void *buf, int n) {
  int tot = 0; const char *p = (const char*)buf; int r;
  while (tot < n && (r = write(fd, (void*)(p + tot), n - tot)) > 0) tot += r;
  return tot == n ? 0 : -1;
}

int
main(int argc, char *argv[])
{
  int p2c[2]; // parent -> child
  int c2p[2]; // child  -> parent
  if(pipe(p2c) < 0 || pipe(c2p) < 0){
    fprintf(2, "pipe failed\n");
    exit(1);
  }

  int parent_pid = getpid();
  int child = fork();
  if(child < 0){
    fprintf(2, "fork failed\n");
    exit(1);
  }

  if(child == 0){
    // 子进程：读取父 pid，然后回复自己的 pid
    close(p2c[1]);
    close(c2p[0]);

    int father_pid;
    if(read_full(p2c[0], &father_pid, sizeof(father_pid)) < 0){
      exit(1);
    }
    printf("%d: received ping from pid %d\n", getpid(), father_pid);

    int self = getpid();
    write_full(c2p[1], &self, sizeof(self));

    close(p2c[0]);
    close(c2p[1]);
    exit(0);
  } else {
    // 父进程：发送自己的 pid，再等待子进程 pid 回来
    close(p2c[0]);
    close(c2p[1]);

    write_full(p2c[1], &parent_pid, sizeof(parent_pid));

    int child_pid;
    if(read_full(c2p[0], &child_pid, sizeof(child_pid)) < 0){
      exit(1);
    }
    printf("%d: received pong from pid %d\n", parent_pid, child_pid);

    close(p2c[1]);
    close(c2p[0]);
    exit(0);
  }
}
