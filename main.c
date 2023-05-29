#define FUSE_USE_VERSION 31
#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

#define MAX_FILE_SIZE 1024
#define MAX_DIR_SIZE 1024
#define MAX_NAME_LEN 1024

struct memfs{
    struct fuse_dir *root;
};

struct memfs * lfs;

struct fuse_file{
    char * id;
    char *content;
    struct stat fstat;
    int content_size;
};

struct fuse_dir{
    char *id;
    struct fuse_file *files[MAX_FILE_SIZE];
    struct fuse_dir *dirs[MAX_DIR_SIZE];
    int file_size;
    int dir_size;
    struct stat dstat;
};
const char * user1 = "bot1";
const char * user2 = "bot2";
struct path{
    char * name[MAX_NAME_LEN];
    int name_num;
};
struct path * spilit_path(const char *path){
    char *path_copy = (char *)malloc(sizeof(char) * MAX_NAME_LEN);
    strcpy(path_copy, path);
    struct path *p = (struct path *)malloc(sizeof(struct path));
    char *token = strtok(path_copy, "/");
    while (token != NULL){
        p->name[p->name_num] = token;
        p->name_num++;
        token = strtok(NULL, "/");
    }
    return p;
}

struct path *chatbot(const struct path *p){
    printf("[chatbot path enter]\n");
    if(p->name_num < 2){
        return NULL;
    }
    struct path *new_p = (struct path *)malloc(sizeof(struct path));
    new_p->name_num = p->name_num;
    for(int i = 0; i < p->name_num; i++){
        new_p->name[i] = (char *)malloc(sizeof(char) * MAX_NAME_LEN);
        strcpy(new_p->name[i], p->name[i]);
    }   
    if(strcmp(p->name[p->name_num-1],user1)==0 && strcmp(p->name[p->name_num-2],user2)==0){
        strcpy(new_p->name[p->name_num-1],user2);
        strcpy(new_p->name[p->name_num-2],user1);
        return new_p;
    }
    else if(strcmp(p->name[p->name_num-1],user2)==0 && strcmp(p->name[p->name_num-2],user1)==0){
        strcpy(new_p->name[p->name_num-1],user1);
        strcpy(new_p->name[p->name_num-2],user2);
        return new_p;
    }
    else{
        return NULL;
    }
}

void add_file(char *name, struct fuse_dir *dir,mode_t  mode){
    printf("[add file %s %s]\n", name, dir->id);
    struct fuse_file *file = (struct fuse_file *)malloc(sizeof(struct fuse_file));
    file->id = name;
    file->content = "";
    file->content_size = 0;
    file->fstat.st_mode = S_IFREG | mode;
    file->fstat.st_nlink = 1;
    file->fstat.st_uid = getuid();
    file->fstat.st_gid = getgid();
    file->fstat.st_atime = time(NULL);
    file->fstat.st_mtime = time(NULL);
    file->fstat.st_ctime = time(NULL);
    dir->files[dir->file_size] = file;
    dir->file_size++;
}

void add_dir(char *name, struct fuse_dir *dir,mode_t mode){
    struct fuse_dir *new_dir = (struct fuse_dir *)malloc(sizeof(struct fuse_dir));
    struct fuse_context *context = fuse_get_context();
    if(context==NULL){
        new_dir->dstat.st_uid = getuid();
        new_dir->dstat.st_gid = getgid();
    }else{
        new_dir->dstat.st_uid = context->uid;
        new_dir->dstat.st_gid = context->gid;
    }
    new_dir->dstat.st_mode = S_IFDIR | mode;
    new_dir->dstat.st_nlink = 2;
    new_dir->dstat.st_atime = time(NULL);
    new_dir->dstat.st_mtime = time(NULL);
    new_dir->dstat.st_ctime = time(NULL);
    new_dir->id = name;
    new_dir->file_size = 0;
    new_dir->dir_size = 0;
    dir->dirs[dir->dir_size] = new_dir;
    dir->dir_size++;
}

void remove_dir(int index, struct fuse_dir * dir){
    struct fuse_dir *temp = dir->dirs[index];
    for(int i = index; i < dir->dir_size - 1; i++){
        dir->dirs[i] = dir->dirs[i+1];
    }
    dir->dir_size--;
    free(temp);
}

void remove_file(int index, struct fuse_dir * dir){
    printf("[remove file]\n");
    printf("%d %d\n", index, dir->file_size);
    // struct fuse_file *temp = dir->files[index];
    for(int i = index; i < dir->file_size - 1; i++){
        dir->files[i] = dir->files[i+1];
    }
    dir->file_size--;
    // free(temp);
}

void init_root_dir(struct fuse_dir *root){
    root->id = "/";
    root->file_size = 0;
    root->dir_size = 0;
    root->dstat.st_mode = S_IFDIR | 0777;
    root->dstat.st_nlink = 2;
    root->dstat.st_uid = getuid();
    root->dstat.st_gid = getgid();
    root->dstat.st_atime = time(NULL);
    root->dstat.st_mtime = time(NULL);
    root->dstat.st_ctime = time(NULL);
}

void init_memfs(struct memfs *memfs){
    memfs->root = (struct fuse_dir *)malloc(sizeof(struct fuse_dir));
    init_root_dir(memfs->root);
}

struct fuse_dir *find_dir(const char *path){
    printf("find_dir [enter] with path: %s\n", path);
    if (strcmp(path, "/") == 0){
        return lfs->root;
    }
    char *token;;
    struct fuse_dir * root = lfs->root;
    struct path * path_token = spilit_path(path);
    for(int i = 0; i < path_token->name_num; i++){
        printf("token: %s\n", path_token->name[i]);
        token = path_token->name[i];
        int max_size = root->dir_size;
        int j;
        for (j = 0; j < root->dir_size; j++){
            printf("%s %s\n", root->dirs[j]->id, token);
            if (strcmp(root->dirs[j]->id, token) == 0){
                printf("FIND!\n");
                printf("%d %d\n", j, root->dir_size);
                root = root->dirs[j];
                break;
            }
        }
        if(j==max_size){
            printf("NOT FIND!\n");
            return NULL;
        } 
    }
    return root;
    // return NULL;
}

struct fuse_file *find_file(const char *path){
    printf("[find_file enter] with path: %s\n", path);
    if (strcmp(path, "/") == 0){
        return NULL;
    }
    char *token;
    struct fuse_dir * root = lfs->root;
    struct path * path_token = spilit_path(path);
    printf("[find file's dir]\n");
    for(int i = 0; i < path_token->name_num-1; ++i){
        token = path_token->name[i];
        int max_size = root->dir_size;
        int j;
        for (j = 0; j < root->dir_size; j++){
            if (strcmp(root->dirs[j]->id, token) == 0){
                root = root->dirs[j];
                break;
            }
        }
        if(j==max_size){
            return NULL;
        }
    }
    printf("[find file]\n");
    for(int i = 0; i < root->file_size; ++i){
        printf("%s %s\n", root->files[i]->id, path_token->name[path_token->name_num-1]);
        if(strcmp(root->files[i]->id, path_token->name[path_token->name_num-1]) == 0){
            return root->files[i];
        }
    }
    return NULL;
}
struct fuse_file *find_file_bypath(struct path * path_token){
    char * token;
    struct fuse_dir * root = lfs->root;
   for(int i = 0; i < path_token->name_num-1; ++i){
        token = path_token->name[i];
        int max_size = root->dir_size;
        int j;
        for (j = 0; j < root->dir_size; j++){
            if (strcmp(root->dirs[j]->id, token) == 0){
                root = root->dirs[j];
                break;
            }
        }
        if(j==max_size){
            return NULL;
        }
    }
    printf("[find file]\n");
    for(int i = 0; i < root->file_size; ++i){
        printf("%s %s\n", root->files[i]->id, path_token->name[path_token->name_num-1]);
        if(strcmp(root->files[i]->id, path_token->name[path_token->name_num-1]) == 0){
            return root->files[i];
        }
    }
    return NULL; 
}

static int fs_getattr(const char *path, struct stat *stbuf,struct fuse_file_info *fi)
{
    printf("[getattr enter] with path: %s\n", path);
    int res = 0;
    struct fuse_dir *dir = find_dir(path);
    struct fuse_file *file = find_file(path);
    if(dir != NULL){
        stbuf->st_mode = dir->dstat.st_mode;
        stbuf->st_nlink = 2;
        stbuf->st_size = 0;
        stbuf->st_atime = time(NULL);
        stbuf->st_mtime = time(NULL);
        stbuf->st_ctime = time(NULL);
        stbuf->st_uid = dir->dstat.st_uid;
        stbuf->st_gid = dir->dstat.st_gid;
    }
    else if(file != NULL){
        stbuf->st_mode = file->fstat.st_mode;
        stbuf->st_nlink = 1;
        stbuf->st_size = file->content_size;
        stbuf->st_atime = time(NULL);
        stbuf->st_mtime = time(NULL);
        stbuf->st_ctime = time(NULL);
        stbuf->st_uid = file->fstat.st_uid;
        stbuf->st_gid = file->fstat.st_gid;
    }
    else{
        printf("[getattr exit] no path find: %s\n", path);
        res = -ENOENT;
    }
    return res;
}

static int fs_access(const char * path, int mask){
    printf("[access enter] with path: %s\n", path);
    struct fuse_file *file = find_file(path);
    struct fuse_dir *dir = find_dir(path);
    if(file != NULL){
        if((mask & file->fstat.st_mode) == mask)
            return 0;
        else
            return -EACCES;
    }
    else if(dir != NULL){
        if((mask & dir->dstat.st_mode) == mask)
            return 0;
        else
            return -EACCES;
    }
    else{
        return -ENOENT;
    }
}

static int fs_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, enum fuse_readdir_flags flags){
    printf("[readdir enter] with path: %s\n", path);
    struct fuse_dir *dir = find_dir(path);
    if(dir == NULL){
        return -ENOENT;
    }
    filler(buf, ".", NULL, 0, 0);
    if (strcmp(path, "/") != 0)
    {
        filler(buf, "..", NULL, 0, 0);
    }
    int i;
    for(i = 0; i < dir->dir_size; i++){
        filler(buf, dir->dirs[i]->id, NULL, 0, 0);
    }
    for(i = 0; i < dir->file_size; i++){
        filler(buf, dir->files[i]->id, NULL, 0, 0);
    }
    return 0;
}

static int fs_mkdir(const char *path, mode_t mode){
    printf("[mkdir enter] with path: %s\n", path);
    
    char *token;
    struct fuse_dir * root = lfs->root;
    struct path * path_token = spilit_path(path);
    //find parent root
    for(int i = 0; i < path_token->name_num-1; ++i){
        token = path_token->name[i];
        int max_size = root->dir_size;
        int j;
        for (j = 0; j < root->dir_size; ++j){
            if (strcmp(root->dirs[j]->id, token) == 0){
                root = root->dirs[j];
                break;
            }
        }
        if(j==max_size){
            return -ENOENT;
        }
    }
    for(int i = 0; i < root->dir_size; ++i){
        if(strcmp(root->dirs[i]->id, path_token->name[path_token->name_num-1]) == 0){
            return -EEXIST;
        }
    }
    add_dir(path_token->name[path_token->name_num-1], root,mode);
    return 0;   
}

static int fs_rmdir(const char *path){
    printf("[rmdir enter] with path: %s\n", path);
    struct fuse_dir *dir = find_dir(path);
    if(dir == NULL){
        return -ENOENT;
    }
    char *token;
    struct fuse_dir * root = lfs->root;
    struct fuse_dir * parent = NULL;
    int index;
    struct path * path_token = spilit_path(path);
    for(int i = 0; i < path_token->name_num; ++i){
        token = path_token->name[i];
        int max_size = root->dir_size;
        int j;
        for (j = 0; j < root->dir_size; ++j){
            if (strcmp(root->dirs[j]->id, token) == 0){
                parent = root;
                index = j;
                root = root->dirs[j];
                break;
            }
        }
        if(j==max_size){
            return -ENOENT;
        }
    }
    remove_dir(index,parent);
    return 0;
}

static int fs_mknod(const char *path, mode_t mode, dev_t dev){
    printf("[mknod enter] with path: %s\n", path);
    struct fuse_file *file = find_file(path);
    if(file != NULL){
        return -EEXIST;
    }
    char *token;
    struct fuse_dir * root = lfs->root;
    struct path * path_token = spilit_path(path);
    for(int i = 0; i < path_token->name_num-1; ++i){
        token = path_token->name[i];
        int max_size = root->dir_size;
        int j;
        for (j = 0; j < root->dir_size; ++j){
            if (strcmp(root->dirs[j]->id, token) == 0){
                root = root->dirs[j];
                break;
            }
        }
        if(j==max_size){
            return -ENOENT;
        }
    }
    for(int i = 0; i < root->file_size; ++i){
        if(strcmp(root->files[i]->id, path_token->name[path_token->name_num-1]) == 0){
            return -EEXIST;
        }
    }
    //add new file
    add_file(path_token->name[path_token->name_num-1], root,mode);
    return 0;
}
//touch
static int fs_create(const char *path, mode_t mode, struct fuse_file_info * fi){
    printf("[create enter] with path: %s\n", path);
    struct fuse_file *file = find_file(path);
    if(file != NULL){
        return -EEXIST;
    }
    char *token;
    struct fuse_dir * root = lfs->root;
    struct path * path_token = spilit_path(path);
    for(int i = 0; i < path_token->name_num-1; ++i){
        token = path_token->name[i];
        int max_size = root->dir_size;
        int j;
        for (j = 0; j < root->dir_size; ++j){
            if (strcmp(root->dirs[j]->id, token) == 0){
                root = root->dirs[j];
                break;
            }
        }
        if(j==max_size){
            return -ENOENT;
        }
    }
    for(int i = 0; i < root->file_size; ++i){
        if(strcmp(root->files[i]->id, path_token->name[path_token->name_num-1]) == 0){
            return -EEXIST;
        }
    }
    //add new file
    add_file(path_token->name[path_token->name_num-1], root,mode);
    return 0;
}

static int fs_unlink(const char *path){
    printf("[unlink enter] with path: %s\n", path);
    struct fuse_file *file = find_file(path);
    if(file == NULL){
        return -ENOENT;
    }
    char *token;
    struct fuse_dir * root = lfs->root;
    int index;
    struct path * path_token = spilit_path(path);
    printf("[unlink find file's dir]\n");
    for(int i = 0; i < path_token->name_num-1; ++i){
        token = path_token->name[i];
        int max_size = root->dir_size;
        int j;
        for (j = 0; j < root->dir_size; ++j){
            if (strcmp(root->dirs[j]->id, token) == 0){
                root = root->dirs[j];
                break;
            }
        }
        if(j==max_size){
            return -ENOENT;
        }
    }
    printf("[unlink find file]\n");
    for(int i = 0; i < root->file_size; ++i){
        if(strcmp(root->files[i]->id, path_token->name[path_token->name_num-1]) == 0){
            printf("FIND FILE\n");
            index = i;
            break;
        }
    }
    remove_file(index,root);
    return 0;
}

static int fs_utimens(const char *path, const struct timespec tv[2],struct fuse_file_info *fi){
    printf("[utimens enter] with path: %s\n", path);
    struct fuse_file *file = find_file(path);
    if(file == NULL){
        return -ENOENT;
    }
    file->fstat.st_atime = tv[0].tv_sec;
    file->fstat.st_mtime = tv[1].tv_sec;
    return 0;
}

static int fs_read(const char *path, char *buf, size_t size, off_t offset,struct fuse_file_info *fi){
    printf("[read enter] with path: %s\n", path);
    struct fuse_file *file = find_file(path);
    if(file == NULL){
        return -ENOENT;
    }
    if(offset > file->content_size){
        return 0;
    }
    if(offset + size > file->content_size){
        size = file->content_size - offset;
    }
    memcpy(buf, file->content + offset, size);
    return size;
}

static int fs_write(const char *path, const char *buf, size_t size, off_t offset,struct fuse_file_info *fi){
    printf("[write enter] with path: %s\n", path);
    struct path * path_token = spilit_path(path);
    printf("[ORIGIN PATH]\n");
    printf("%s\n",path_token->name[path_token->name_num-1]);
    printf("%s\n",path_token->name[path_token->name_num-2]);
    struct path * chatp = chatbot(path_token);
    printf("[CHATP PATH]\n");
    printf("%s\n",chatp->name[chatp->name_num-1]);
    printf("%s\n",chatp->name[chatp->name_num-2]);
    
    if(chatp != NULL){
        printf("[chatbot enter]\n");
        struct fuse_file *file1 = find_file_bypath(path_token);
        struct fuse_file *file2 = find_file_bypath(chatp);
        for(int i = 0; i < 2; ++i){
            if(i==0){
                if(file1 == NULL){
                    char * token;
                    struct fuse_dir * root = lfs->root;
                    for(int i = 0; i < path_token->name_num-1; ++i){
                        token = path_token->name[i];
                        int max_size = root->dir_size;
                        int j;
                        for (j = 0; j < root->dir_size; ++j){
                            if (strcmp(root->dirs[j]->id, token) == 0){
                                root = root->dirs[j];
                                break;
                            }
                        }
                        if(j==max_size){
                            return -ENOENT;
                        }
                    }
                    mode_t mode = S_IFREG | 0666;
                    printf("###add origin file###\n");
                    add_file(path_token->name[path_token->name_num-1], root,mode); 
                    file1 = find_file_bypath(path_token);
                }
                if(file1->content_size == 0){
                        file1->content = (char *)malloc(size);
                        file1->content_size = size;
                }
                else if(offset + size > file1->content_size){
                        file1->content = (char *)realloc(file1->content, offset + size);
                        file1->content_size = offset + size;
                }
                memcpy(file1->content + offset, buf, size);
            }
            else if(i==1){
                if(file2 == NULL){
                    char * token;
                    struct fuse_dir * root = lfs->root;
                    for(int i = 0; i < chatp->name_num-1; ++i){
                        token = chatp->name[i];
                        int max_size = root->dir_size;
                        int j;
                        for (j = 0; j < root->dir_size; ++j){
                            if (strcmp(root->dirs[j]->id, token) == 0){
                                root = root->dirs[j];
                                break;
                            }
                        }
                        if(j==max_size){
                            return -ENOENT;
                        }
                    }
                    mode_t mode = S_IFREG | 0666;
                    printf("###add chatp file###\n");
                    add_file(chatp->name[chatp->name_num-1], root,mode); 
                    file2 = find_file_bypath(chatp);
                }
                
                if(file2->content_size == 0){
                        file2->content = (char *)malloc(size);
                        file2->content_size = size;
                }
                else if(offset + size > file2->content_size){
                        file2->content = (char *)realloc(file2->content, offset + size);
                        file2->content_size = offset + size;
                }
                memcpy(file2->content + offset, buf, size);
            }
        }
        return size;                                       
    }
    struct fuse_file *file = find_file(path);
    if(file == NULL){
        struct path * path_token = spilit_path(path);
        char * token;
        struct fuse_dir * root = lfs->root;
        for(int i = 0; i < path_token->name_num-1; ++i){
            token = path_token->name[i];
            int max_size = root->dir_size;
            int j;
            for (j = 0; j < root->dir_size; ++j){
                if (strcmp(root->dirs[j]->id, token) == 0){
                    root = root->dirs[j];
                    break;
                }
            }
            if(j==max_size){
                return -ENOENT;
            }
        }
        mode_t mode = S_IFREG | 0666;
        add_file(path_token->name[path_token->name_num-1], root,mode);
    }
    if(file->content_size == 0){
        file->content = (char *)malloc(size);
        file->content_size = size;
    }
    else if(offset + size > file->content_size){
        file->content = (char *)realloc(file->content, offset + size);
        file->content_size = offset + size;
    }
    memcpy(file->content + offset, buf, size);
    return size;
}

static int fs_chmod(const char *path, mode_t mode,struct fuse_file_info *fi){
    printf("[chmod enter] with path: %s\n", path);
    struct fuse_file *file = find_file(path);
    if(file == NULL){
        return -ENOENT;
    }
    file->fstat.st_mode = mode;
    return 0;
}

static int fs_chown(const char *path, uid_t uid, gid_t gid,struct fuse_file_info *fi){
    printf("[chown enter] with path: %s\n", path);
    struct fuse_file *file = find_file(path);
    if(file == NULL){
        return -ENOENT;
    }
    file->fstat.st_uid = uid;
    file->fstat.st_gid = gid;
    return 0;
}

static struct fuse_operations memfs_oper = {
    .getattr = fs_getattr, 
    .readdir = fs_readdir,
    .access = fs_access,
    .mkdir = fs_mkdir,
    .rmdir = fs_rmdir,
    .mknod = fs_mknod,
    .create = fs_create,
    .unlink = fs_unlink,
    .utimens = fs_utimens,
    .read = fs_read,
    .write = fs_write,
    .chmod = fs_chmod,
    .chown = fs_chown,
};

int main(int argc, char *argv[])
{
    int ret;
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

    lfs = (struct memfs *)malloc(sizeof(struct memfs));
    init_memfs(lfs);


    ret = fuse_main(args.argc, args.argv, &memfs_oper, NULL);
    fuse_opt_free_args(&args);
    return ret;
}