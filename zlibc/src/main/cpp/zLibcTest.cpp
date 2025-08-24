#include "zLog.h"
#include "zLibc.h"

// 测试字符串函数
void test_string_functions() {
    LOGI("=== Testing String Functions ===");

    // 测试 strlen
    const char* test_str = "Hello World";
    size_t len = strlen(test_str);
    LOGI("strlen('%s') = %zu", test_str, len);

    // 测试 strcmp
    const char* str1 = "hello";
    const char* str2 = "hello";
    const char* str3 = "world";
    int result1 = strcmp(str1, str2);
    int result2 = strcmp(str1, str3);
    LOGI("strcmp('%s', '%s') = %d", str1, str2, result1);
    LOGI("strcmp('%s', '%s') = %d", str1, str3, result2);

    // 测试 strcpy
    char dest[50];
    const char* src = "Test String";
    char* copied = strcpy(dest, src);
    LOGI("strcpy result: '%s'", copied);

    // 测试 strcat
    char cat_dest[50] = "Hello ";
    const char* cat_src = "World";
    char* cat_result = strcat(cat_dest, cat_src);
    LOGI("strcat result: '%s'", cat_result);

    // 测试 strncmp
    const char* nstr1 = "hello";
    const char* nstr2 = "help";
    int nresult = strncmp(nstr1, nstr2, 3);
    LOGI("strncmp('%s', '%s', 3) = %d", nstr1, nstr2, nresult);
}

// 测试内存函数
void test_memory_functions() {
    LOGI("=== Testing Memory Functions ===");

    // 测试 malloc
    void* ptr1 = malloc(100);
    if (ptr1) {
        LOGI("malloc(100) = %p", ptr1);

        // 测试 memset
        void* memset_result = memset(ptr1, 0xAA, 100);
        LOGI("memset result = %p", memset_result);

        // 测试 memcpy
        void* ptr2 = malloc(100);
        if (ptr2) {
            void* memcpy_result = memcpy(ptr2, ptr1, 100);
            LOGI("memcpy result = %p", memcpy_result);

            // 测试 memcmp
            int memcmp_result = memcmp(ptr1, ptr2, 100);
            LOGI("memcmp result = %d", memcmp_result);

            free(ptr2);
        }

        free(ptr1);
    }

    // 测试 calloc
    void* calloc_ptr = calloc(10, 20);
    if (calloc_ptr) {
        LOGI("calloc(10, 20) = %p", calloc_ptr);
        free(calloc_ptr);
    }

    // 测试 realloc
    void* realloc_ptr = malloc(50);
    if (realloc_ptr) {
        LOGI("malloc(50) = %p", realloc_ptr);
        void* realloc_result = realloc(realloc_ptr, 100);
        LOGI("realloc(100) = %p", realloc_result);
        if (realloc_result) {
            free(realloc_result);
        }
    }
}

// 测试文件操作函数
void test_file_functions() {
    LOGI("=== Testing File Functions ===");

    const char* test_file = "/data/local/tmp/test.txt";
    const char* test_content = "Hello from zlibc test!\n";

    // 测试 open (创建文件)
    int fd = open(test_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) {
        // 检查文件描述符是否在标准范围内（0-2）
        if (fd <= 2) {
            LOGE("test_file_functions: WARNING - File descriptor %d is in standard range (0-2) for file %s", fd, test_file);
            LOGE("test_file_functions: This may cause issues with Android's unique_fd management");
        }
        LOGI("open('%s') = %d", test_file, fd);

        // 测试 write
        ssize_t write_result = write(fd, test_content, strlen(test_content));
        LOGI("write result = %zd", write_result);

        // 测试 close
        int close_result = close(fd);
        LOGI("close result = %d", close_result);

        // 测试 open (读取文件)
        fd = open(test_file, O_RDONLY);
        if (fd >= 0) {
            // 检查文件描述符是否在标准范围内（0-2）
            if (fd <= 2) {
                LOGE("test_file_functions: WARNING - File descriptor %d is in standard range (0-2) for file %s", fd, test_file);
                LOGE("test_file_functions: This may cause issues with Android's unique_fd management");
            }
            LOGI("open('%s') for reading = %d", test_file, fd);

            // 测试 read
            char read_buffer[100];
            ssize_t read_result = read(fd, read_buffer, sizeof(read_buffer) - 1);
            if (read_result > 0) {
                read_buffer[read_result] = '\0';
                LOGI("read result = %zd, content = '%s'", read_result, read_buffer);
            }

            // 测试 fstat
            struct stat st;
            int fstat_result = fstat(fd, &st);
            LOGI("fstat result = %d, file size = %ld", fstat_result, (long)st.st_size);

            // 测试 lseek
            off_t lseek_result = lseek(fd, 0, SEEK_SET);
            LOGI("lseek result = %ld", (long)lseek_result);

            // 避免关闭标准文件描述符（0-2）
            if (fd > 2) {
                close(fd);
            } else {
                LOGE("test_file_functions: WARNING - Skipping close of standard file descriptor %d (stdin/stdout/stderr)", fd);
                LOGE("test_file_functions: This prevents conflict with Android's unique_fd management");
            }
        }

        // 测试 access
        int access_result = access(test_file, R_OK | W_OK);
        LOGI("access('%s') = %d", test_file, access_result);

        // 测试 stat
        struct stat st;
        int stat_result = stat(test_file, &st);
        LOGI("stat('%s') = %d, mode = %o", test_file, stat_result, st.st_mode);

        // 清理测试文件
        unlink(test_file);
    } else {
        LOGE("Failed to open test file");
    }
}

// 测试时间函数
void test_time_functions() {
    LOGI("=== Testing Time Functions ===");

    // 测试 time
    time_t current_time = time(NULL);
    LOGI("time() = %ld", (long)current_time);

    // 测试 gettimeofday
    struct timeval tv;
    struct timezone tz;
    int gettimeofday_result = gettimeofday(&tv, &tz);
    LOGI("gettimeofday result = %d, tv_sec = %ld, tv_usec = %ld",
         gettimeofday_result, (long)tv.tv_sec, (long)tv.tv_usec);

    // 测试 localtime
    struct tm* tm_result = localtime(&current_time);
    if (tm_result) {
        LOGI("localtime");
    }
}

// 测试进程函数
void test_process_functions() {
    LOGI("=== Testing Process Functions ===");

    // 测试 getpid
    pid_t pid = getpid();
    LOGI("getpid() = %d", pid);

    // 测试 getppid
    pid_t ppid = getppid();
    LOGI("getppid() = %d", ppid);
}

// 测试其他函数
void test_other_functions() {
    LOGI("=== Testing Other Functions ===");

    // 测试 atoi
    const char* num_str1 = "123";
    const char* num_str2 = "-456";
    const char* num_str3 = "abc";

    int atoi_result1 = atoi(num_str1);
    int atoi_result2 = atoi(num_str2);
    int atoi_result3 = atoi(num_str3);

    LOGI("atoi('%s') = %d", num_str1, atoi_result1);
    LOGI("atoi('%s') = %d", num_str2, atoi_result2);
    LOGI("atoi('%s') = %d", num_str3, atoi_result3);

    // 测试 atol
    const char* long_str = "123456789";
    long atol_result = atol(long_str);
    LOGI("atol('%s') = %ld", long_str, atol_result);

    // 测试 strchr
    const char* search_str = "Hello World";
    const char* strchr_result = strchr(search_str, 'W');
    if (strchr_result) {
        LOGI("strchr('%s', 'W') = '%s'", search_str, strchr_result);
    }

    // 测试 strrchr
    const char* rsearch_str = "Hello World";
    const char* strrchr_result = strrchr(rsearch_str, 'l');
    if (strrchr_result) {
        LOGI("strrchr('%s', 'l') = '%s'", rsearch_str, strrchr_result);
    }
}

// 测试网络函数
void test_network_functions() {
    LOGI("=== Testing Network Functions ===");

    // 测试 socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock >= 0) {
        LOGI("socket(AF_INET, SOCK_STREAM, 0) = %d", sock);

        // 测试 fcntl
        int fcntl_result = fcntl(sock, F_GETFL);
        LOGI("fcntl(F_GETFL) = %d", fcntl_result);

        close(sock);
    } else {
        LOGE("socket creation failed");
    }
}

void __attribute__((constructor)) init_(void){
    LOGI("zlibc init - Starting comprehensive tests");

    // 运行所有测试
    test_string_functions();
    test_memory_functions();
    test_file_functions();
    test_time_functions();
    test_process_functions();
    test_other_functions();
    test_network_functions();

    LOGI("zlibc init - All tests completed");
}