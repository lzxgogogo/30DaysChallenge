
#define _GNU_SOURCE

#include <dlfcn.h>
#include <stdio.h>
#include <ucontext.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>


ucontext_t ctx[3];

ucontext_t main_ctx;
int count = 0;

// hook

typedef ssize_t (*read_t)(int fd, void *buf, size_t count);
read_t read_f = NULL;

typedef ssize_t (* write_t)(int fd, const void *buf, size_t count);
write_t write_f = NULL;

ssize_t read(int fd, void *buf, size_t count){
    
    return read_f(fd, buf, count);
}

ssize_t write(int fd, const void *buf, size_t count){
    return write_f(fd, buf, count);
}

void init_hook(void)
{
    if (!read_f) {
        read_f = dlsym(RTLD_NEXT, "read");
    }

    if(! write_f){
        write_f = dlsym(RTLD_NEXT, "write");
    }
}

void func1(void){
    while (count ++ < 30) {
		printf("1: 发起请求\n");
		//swapcontext(&ctx[0], &ctx[1]);
		swapcontext(&ctx[0], &main_ctx);
		printf("1: 得到处理\n");
	}
}

void func2(void){
    while (count ++ < 30) {
		printf("2: 发起请求\n");
		//swapcontext(&ctx[1], &ctx[2]);
		swapcontext(&ctx[1], &main_ctx);
		printf("2: 得到处理\n");
	}

}

void func3(void){
    while (count ++ < 30) {
		printf("3: 发起请求\n");
		//swapcontext(&ctx[2], &ctx[0]);
		swapcontext(&ctx[2], &main_ctx);
		printf("3: 得到处理\n");
	}

}

int main(){
    
    init_hook();
    
    int fd = open("a.txt", O_CREAT | O_RDWR,0644);
    if(fd < 0){
        return -1;
    }

    char * str = "1234567890";
    int ret = write(fd, str, strlen(str));
    printf("ret: %d\n",ret);

    // 修正2：重置文件指针到开头
    lseek(fd, 0, SEEK_SET);
    
    char buffer[128] = {0};
    read(fd, buffer, 128);
    
    printf("buffer: %s\n", buffer);

#if 0    
    char stack1[2048] = {0};
    char stack2[2048] = {0};
    char stack3[2048] = {0};

    getcontext(&ctx[0]);
    ctx[0].uc_stack.ss_sp = stack1;
    ctx[0].uc_stack.ss_size = sizeof(stack1);
    ctx[0].uc_link = &main_ctx;
    makecontext(&ctx[0], func1, 0);
    
    getcontext(&ctx[1]);
    ctx[1].uc_stack.ss_sp = stack2;
    ctx[1].uc_stack.ss_size = sizeof(stack2);
    ctx[1].uc_link = &main_ctx;
    makecontext(&ctx[1], func2, 0);
    
    getcontext(&ctx[2]);
    ctx[2].uc_stack.ss_sp = stack3;
    ctx[2].uc_stack.ss_size = sizeof(stack3);
    ctx[2].uc_link = &main_ctx;
    makecontext(&ctx[2], func3, 0);   

    printf("swapcontext\n");
    while(count <= 30){
        swapcontext(&main_ctx, &ctx[count%3]);
    }
    printf("\n");
#endif 
}