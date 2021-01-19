#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#include <semaphore.h>
#include <stack>
#include <cstring>
#include <fcntl.h>
#include <cstdlib>
#include <vector>
#include <sys/wait.h>

#define NUM_OF_THREADS 10
using namespace std;
sem_t my_sem;

struct FileOperation
{
    string src_file;
    string dst_file;
};
int times = 0;
stack<FileOperation> homework;

//复制文件
void cp(string src_file, string dst_file)
{
    int fd_read = open(src_file.c_str(), O_RDONLY, S_IREAD | S_IWRITE | S_IRGRP | S_IROTH);
    int fd_write = open(dst_file.c_str(), O_WRONLY | O_CREAT, S_IREAD | S_IWRITE | S_IRGRP | S_IWGRP | S_IROTH);
    cout << dst_file.c_str() << endl;
    cout << src_file.c_str() << endl;
    cout << fd_read << ":" << fd_write << endl;
    cout << src_file << ":" << dst_file << endl;
    if (fd_read == -1 || fd_write == -1)
    {
        cout << "when copy " << src_file << "复制失败 !" << endl;
        cout << "failed:" << times << endl;
        times++;
    }
    else
    {
        char buf[1024];
        int size = 0;
        while ((size = read(fd_read, buf, 1024)) > 0)
        {
            write(fd_write, buf, size);
        }
        cout << "file:" << src_file << "复制成功 !" << endl;
    }
    close(fd_write);
    close(fd_read);
    cout << "总失败次数" << times << endl;
}

//遍历目录
void walk_dir(const char *src_dir, const char *dst_dir)
{
    struct dirent *filename;
    DIR *dir;
    dir = opendir(src_dir); //获得目录信息
    if (dir == NULL)
    {
        cout << "打开 src_dir 失败" << endl;
        exit(0); //结束当前进程
    }
    cout << "open src_dir success!" << endl;
    char path[256];
    while ((filename = readdir(dir)) != NULL)
    { //读取文件夹下文件
        if ((!strcmp(filename->d_name, ".")) || (!strcmp(filename->d_name, "..")))
        {
            continue; //遇到. ..就跳过
        }
        snprintf(path, 256, "%s/%s", src_dir, filename->d_name); //按照"%S"格式储存到path中
        struct stat s;
        lstat(path, &s); //获取path详细信息储存进s
        if (S_ISDIR(s.st_mode))
        { // S_ISDIR()函数 判断一个路径是否为目录
            char sub_src_dir[256];
            char sub_dst_dir[256];
            snprintf(sub_dst_dir, 256, "%s/%s", dst_dir, filename->d_name);
            snprintf(sub_src_dir, 256, "%s/%s", src_dir, filename->d_name);
            cout << sub_dst_dir << endl;
            mkdir(sub_dst_dir, S_IWUSR | S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH); //!!竟然把dst写成了src
            walk_dir(sub_src_dir, sub_dst_dir);
        }
        else
        { //是文件
            char dst_file[256];
            char src_file[256];
            snprintf(dst_file, 256, "%s/%s", dst_dir, filename->d_name);
            snprintf(src_file, 256, "%s/%s", src_dir, filename->d_name);
            struct FileOperation new_operation;
            new_operation.src_file = src_file;
            new_operation.dst_file = dst_file;
            homework.push(new_operation);
        }
    }
    closedir(dir);
}

void *run(void *)
{
    struct FileOperation opreation;
    while (!homework.empty())
    {
        sem_wait(&my_sem);
        opreation.dst_file = homework.top().dst_file;
        opreation.src_file = homework.top().src_file;
        homework.pop();
        sem_post(&my_sem); //释放锁
        cp(opreation.src_file, opreation.dst_file);
    }
    return NULL;
}
int main(int argc, char *argv[])
{
    sem_init(&my_sem, 0, 1);
    if (argc < 3)
    {
        cout << "please give right path" << endl;
        exit(0);
    }
    struct stat s;
    //检查文件夹是否有校
    lstat(argv[1], &s);
    if (!S_ISDIR(s.st_mode))
    {
        cout << "the source path is wrong" << endl;
        exit(0);
    }
    //检查目标文件夹是否有效;
    lstat(argv[2], &s);
    if (!S_ISDIR(s.st_mode))
    {
        cout << "the dest path is wrong" << endl;
    }
    walk_dir(argv[1], argv[2]);
    vector<pthread_t> threads;
    threads.resize(NUM_OF_THREADS); //设置线程数目
    for (int i = 0; i < threads.size(); i++)
    {
        pthread_create(&threads[i], NULL, run, NULL);
    }
    for (int i = 0; i < threads.size(); i++)
    {
        pthread_join(threads[i], NULL);
    }
    return 0;
}
// 另一种copy文件的方式
// int main(int argc,char *argv[])
// {
//     int fds,fdt,n;
//     char buf[N];
//     if(argc < 3)
//     {
//         printf("usage : %s <src_file><dst_file>\n",argv[0]);
//         return -1;
//     }

//     if((fds = open(argv[1],O_RDONLY))==-1)//证明没打开
//     {
//         fprintf(stderr,"open %s :%s\n",argv[1],strerror(errno));//标准错误流中传信息
//         return -1;
//     }
//     if((fdt = open(argv[2],O_WRONLY|O_CREAT|O_TRUNC,0666))==-1)//证明没打开
//     {
//         fprintf(stderr,"open %s :%s\n",argv[2],strerror(errno));//标准错误流中传信息
//         return -1;
//     }
//     while((n =read(fds,buf,N))>0)
//     {
//         write(fdt,buf,n);
//     }
//     close(fds);
//     close(fdt);
//     return 0;
// }