#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "thrd_pool.h"  // 包含线程池头文件

// 任务处理函数示例：打印传入的消息
void print_message(void *arg) {
    char *msg = (char *)arg;
    printf("Thread %lu: %s\n", (unsigned long)pthread_self(), msg);
    usleep(100000); // 模拟耗时操作（100ms）
}

// 测试终止后提交任务的函数
void test_post_after_terminate(thrdpool_t *pool) {
    printf("\n尝试在终止后提交任务...\n");
    int ret = thrdpool_post(pool, print_message, "This should fail!");
    if (ret != 0) {
        printf("提交失败（预期行为），错误码: %d\n", ret);
    }
}

int main() {
    // 1. 创建线程池（4个工作线程）
    printf("创建线程池...\n");
    thrdpool_t *pool = thrdpool_create(4);
    if (!pool) {
        fprintf(stderr, "线程池创建失败！\n");
        return 1;
    }

    // 2. 提交多个任务
    printf("提交10个任务...\n");
    char *messages[] = {
        "Task 1: Hello World!",
        "Task 2: 你好，世界！",
        "Task 3: Bonjour le monde!",
        "Task 4: Hola Mundo!",
        "Task 5: こんにちは世界！",
        "Task 6: Hallo Welt!",
        "Task 7: Ciao mondo!",
        "Task 8: 안녕하세요 세상!",
        "Task 9: Здравствуй, мир!",
        "Task 10: สวัสดีโลก!"
    };

    for (int i = 0; i < 10; i++) {
        int ret = thrdpool_post(pool, print_message, messages[i]);
        if (ret != 0) {
            fprintf(stderr, "任务提交失败！错误码: %d\n", ret);
        }
    }

    // 3. 等待所有任务完成
    printf("\n等待任务执行...\n");
    sleep(2);  // 等待2秒确保大部分任务完成

    // 4. 终止线程池并测试终止后提交
    printf("\n终止线程池...\n");
    thrdpool_terminate(pool);
    test_post_after_terminate(pool);  // 预期失败

    // 5. 等待线程池完全关闭
    printf("\n等待线程池资源释放...\n");
    thrdpool_waitdone(pool);

    printf("\n所有测试完成！\n");
    return 0;
}