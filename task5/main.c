#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include<string.h>

int main(){
    
    pid_t pid;
    // OPEN FILES
    int fd;
    fd = open("test.txt" , O_RDWR | O_CREAT | O_TRUNC);
    printf("qwq1\n");
    if (fd == -1)
    {
        /* code */
        printf("fail to open a file!\n");
        return -1;
    }
    //write 'hello fcntl!' to file
printf("qwq2\n");
    /* code */
    
    char * mmaped = NULL;
    char *str = "hello fcntl!";
    int len = strlen(str);
    printf("%d\n",len);
    ftruncate(fd,len);
    mmaped = (char *)mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    printf("%s\n",str);
    memcpy(mmaped,str,strlen(str));
    munmap(mmaped,len);

    // DUPLICATE FD

    /* code */
    int new_fd = dup(fd);
    
    

    pid = fork();

    if(pid < 0){
        // FAILS
        printf("error in fork");
        return 1;
    }
    
    struct flock fl;

    if(pid > 0){
        // PARENT PROCESS
        //set the lock
        struct flock file_lock;
        file_lock.l_type = F_WRLCK;
        file_lock.l_whence = SEEK_SET;
        file_lock.l_start = 0;
        file_lock.l_len = 0; 
        int signal = fcntl(fd, F_SETLK, &file_lock);
        if (signal==-1){
            printf("fail lock file\n");
            return -1;
        }
        //append 'b'
        off_t file_size = lseek(fd, 0, SEEK_END);
        size_t size = file_size + 1;
        ftruncate(fd, size);
        mmaped = (char *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        mmaped[size-1] = 'b';
        // mmaped[size-1] = '\0';
        munmap(mmaped,size);
        //unlock

        file_lock.l_type = F_UNLCK;
        signal = fcntl(fd,F_SETLK,&file_lock);
        if(signal == -1){
            printf("fail unlock file\n");
            return -1;
        }
        sleep(3);

        // printf("%s", str); the feedback should be 'hello fcntl!ba'
        
        exit(0);

    } else {
        // CHILD PROCESS
        sleep(2);
        //get the lock
        struct flock file_lock;
        file_lock.l_type = F_WRLCK;
        file_lock.l_whence = SEEK_SET;
        file_lock.l_start = 0;
        file_lock.l_len = 0;
        int signal = fcntl(new_fd,F_GETLK,&file_lock);
        if(file_lock.l_type == F_UNLCK){
            off_t file_size = lseek(new_fd, 0, SEEK_END);
            size_t size = file_size + 1;
            ftruncate(new_fd, size);
            mmaped = mmap(NULL,size,PROT_READ | PROT_WRITE, MAP_SHARED, new_fd, 0);
            mmaped[size-1] = 'a';
            // mmaped[size-1] = '\0'; 
            munmap(mmaped,size);
            close(new_fd);
        }else {
            printf("still lock in child process\n");
        }
        //append 'a'
        

        exit(0);
    }
    munmap(mmap,len);
    close(fd);
    return 0;
}