#define FUSE_USE_VERSION 26

#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <fuse.h>
#include <limits.h>
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>

#define MAXSIZE 1000
#define FILE_INIT 0
typedef struct mystat 
{
    mode_t st_mode;
    nlink_t st_nlink;
    uid_t  st_uid;
    gid_t st_gid;
    off_t st_size;
    time_t a_time;
    time_t m_time;
    time_t c_time;

} mystat;


typedef struct fileNode
{
    int    parentDir;
    int childDir;
    int next;
    char    name[255];
    struct mystat statCont1;
    char    data[500];
    int     type; //0 - default/1 - dir/2 - file
} fsNode;


fsNode file[MAXSIZE];
int bitmap[MAXSIZE] = {0};
//0 is empty 1 is occupied

long memory;
//0th block is root node
fsNode *fsroot=file+0;
fsNode *fsend=file+MAXSIZE-1;


void do_file_write() {
    int i;
    printf("writing\n");
	FILE *fp = fopen("data","wb");
	for(i=0;i<MAXSIZE;i++){
		fwrite(&bitmap[i],sizeof(int),1,fp);
	}
	for(i=0;i<MAXSIZE;i++){
		fwrite(&file[i],sizeof(fsNode),1,fp);
	}
	fclose(fp);

}

void do_file_read(){
	int i;
    printf("reading");
	FILE *fp = fopen("data","rb");
	for(i=0;i<MAXSIZE;i++){
		fread(&bitmap[i],sizeof(int),1,fp);
	}
	for(i=0;i<MAXSIZE;i++){
		fread(&file[i],sizeof(fsNode),1,fp);
	}
	fclose(fp);
}

fsNode* find_free_index() {
    int i;
    for(i = 0; i < MAXSIZE; i++){
        if(bitmap[i] == 0){
			bitmap[i]=1;
            return file+i;
        }
    }
    return NULL;
}

static int check_path(const char *path) {
    if(strlen(path)  > PATH_MAX)
        return 0;
printf("\ncheck path fine");    
return 1;
}

char currentNode[255];

int check_for_path(const char *path) {
	printf("Check for path function\n");
    int check = 0;
    fsNode *tempPtr  = fsroot;
    fsNode *tempPtr2 = fsend;
    char tempPath[PATH_MAX];
    char *tempPath2;

    strcpy(tempPath, path);
    tempPath2 = strtok(tempPath, "/");

    if((tempPath2==NULL)&&(strcmp(path, "/")==0))
        return 0;

    while(tempPath2 != NULL) {
        
		for(tempPtr2 = file+tempPtr->childDir; tempPtr2 != fsend; tempPtr2 = file+tempPtr2->next) {
            if(strcmp(tempPtr2->name, tempPath2) == 0 ) {
                check = 1;
                break;
            }
        }


        tempPath2 = strtok(NULL, "/");
        if(check!=1){
            if(tempPath2==NULL)
                return 1;
            else
                return -1;
        } else {
            if((check==1)&&(tempPath2==NULL))
                return 0;
        }
        tempPtr = tempPtr2;
        check = 0;
    }
    return -1;
}

fsNode *find_file_node(const char *path) {

    int check;
    fsNode *tempPtr = fsroot;
    fsNode *tempPtr2 = fsend;
    char *tempPath2;
    char tempPath[PATH_MAX];

    strcpy(tempPath, path);
    tempPath2 = strtok(tempPath, "/");

    if((tempPath2==NULL)&&(strcmp(path, "/")==0))
        return fsroot;

    while(tempPath2!=NULL) {
        check = 0;
        for(tempPtr2=file+tempPtr->childDir; tempPtr2!=fsend; tempPtr2=file+tempPtr2->next) {
            if(strcmp(tempPtr2->name, tempPath2)==0){
                check = 1;
                break;
            }
        }
        if(check!=1) {
            strcpy(currentNode,tempPath2);
            return tempPtr;
        }else {
            tempPath2 = strtok(NULL, "/");
            if(tempPath2==NULL)
                return tempPtr2;
        }
        tempPtr=tempPtr2;
    }
    return NULL;
}

static int ramd_unlink(const char *path) {
    
    if (!check_path(path)) return -ENAMETOOLONG;
    
    if(check_for_path(path)==0) {
        fsNode *tempPtr = find_file_node(path);
        fsNode *tempPtr2 = file+tempPtr->parentDir;
		
		printf("tempPtr index=%ld\n",tempPtr-file);
        if(file+tempPtr2->childDir==tempPtr && tempPtr->next==MAXSIZE-1) {
			//bitmap[tempPtr-file]=0;            
			tempPtr2->childDir = MAXSIZE-1;
        } else if (file+tempPtr2->childDir==tempPtr) {
			//bitmap[tempPtr-file]=0;            			
			tempPtr2->childDir = tempPtr->next;
        } else {
            fsNode *temporary;
            temporary = file+tempPtr2->childDir;
            while(temporary != fsend) {
                if(file+temporary->next == tempPtr) {
					//bitmap[tempPtr-file]=0;
                    temporary->next = tempPtr->next;
                    break;
                } else {
                    temporary = file+temporary->next;
                }
            }    
        }

        if(tempPtr->statCont1.st_size!=0) {
            memory = memory + tempPtr->statCont1.st_size;
			//free(tempPtr->data);
            //free(tempPtr->statCont1);
            //free(tempPtr);
        } else {
            //free(tempPtr->statCont1);
            //free(tempPtr);
        }
        do_file_write();
        return 0;
    }
    return -ENOENT;
}

static int ramd_mkdir(const char *path, mode_t mode) {

    if (!check_path(path)) return -ENAMETOOLONG;

//  fsNode *tempPtr2 = (fsNode *)malloc(sizeof(fsNode));
	fsNode *tempPtr2 = find_free_index();
    if(tempPtr2==NULL)
        return -ENOSPC;

    fsNode *tempPtr = find_file_node(path);

/*  if(memory < 0) {
        return -ENOSPC;
    }
*/

    time_t current;
    time(&current);
    strcpy(tempPtr2->name, currentNode);

    tempPtr2->statCont1.st_nlink = 2;
    tempPtr2->statCont1.st_mode = S_IFDIR | 0755;
    tempPtr2->statCont1.st_uid = getuid();
    tempPtr2->statCont1.st_gid = getgid();
    tempPtr2->statCont1.a_time = current;
    tempPtr2->statCont1.m_time = current;
    tempPtr2->statCont1.c_time = current;
    tempPtr2->parentDir = tempPtr-file;
    tempPtr2->childDir = MAXSIZE-1;
    tempPtr2->next = MAXSIZE-1;
    tempPtr2->statCont1.st_size = 4096;
    tempPtr2->type = 1;

    if(tempPtr->childDir==MAXSIZE-1) {
        tempPtr->childDir = tempPtr2-file;
        tempPtr->statCont1.st_nlink += 1;
    } else {
        fsNode *temporary = file+tempPtr->childDir;
        while(temporary->next != MAXSIZE-1) {
            temporary = file+temporary->next;
        }
        temporary->next = tempPtr2-file;
        tempPtr->statCont1.st_nlink += 1;
    }
    do_file_write();
    return 0;
}

static int ramd_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    
    if (!check_path(path)) return -ENAMETOOLONG;

    if(check_for_path(path) != -1) {
        fsNode *tempPtr=find_file_node(path);
        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);
        
        fsNode *temporary;
        for(temporary=file+tempPtr->childDir;temporary!=fsend; temporary=file+temporary->next)
            filler(buf, temporary->name, NULL, 0);

        time_t current;
        time(&current);
        tempPtr->statCont1.a_time = current;
        do_file_write();
        return 0;
    }
    return -ENOENT;
}

static int ramd_rmdir(const char *path) {
    
    if (!check_path(path)) return -ENAMETOOLONG;

    if(check_for_path(path)==0) {
        fsNode *tempPtr = find_file_node(path);
        if(tempPtr->childDir!=MAXSIZE-1)
            return -ENOTEMPTY;
        printf("In rmdir\n");
        fsNode *temporary = file+tempPtr->parentDir;
		printf("parent->childDir=%d   temp->next=%d\n",temporary->childDir,tempPtr->next);
		printf("Index of tempPtr=%ld\n",tempPtr-file);
        if(file+temporary->childDir==tempPtr && tempPtr->next==MAXSIZE-1){
			bitmap[tempPtr-file]=0;
            temporary->childDir = MAXSIZE-1;}
        else if(file+temporary->childDir==tempPtr){
			bitmap[tempPtr-file]=0;
            temporary->childDir = tempPtr->next;}
        else {
            fsNode *temporary2;
            for(temporary2=file+temporary->childDir; temporary2!=fsend; temporary2=file+temporary2->next) {
                if(file+temporary2->next==tempPtr) {
                    temporary2->next = tempPtr->next;
					bitmap[tempPtr-file]=0;
                    break;
                }
            }
        }
		printf("\nbitmap %ld: %d",tempPtr-file,bitmap[tempPtr-file]);
        temporary->statCont1.st_nlink--;

        do_file_write();
        return 0;
    }

    return -ENOENT;
}

static int ramd_create(const char *path, mode_t mode, struct fuse_file_info *fi) {

    if (!check_path(path)) return -ENAMETOOLONG;

    if(memory < 0) {
        return -ENOMEM;
    }

//  fsNode *tempPtr=(fsNode *)malloc(sizeof(fsNode));
	fsNode *tempPtr=find_free_index();
	printf("Created file at %ld\n",tempPtr-file);
    fsNode *tempPtr2 = find_file_node(path);
//  tempPtr->statCont1 = (mystat *)malloc(sizeof(mystat));
    if(tempPtr == NULL)
        return -ENOMEM;

    strcpy(tempPtr->name, currentNode);
    tempPtr->statCont1.st_mode = S_IFREG | mode;
    tempPtr->statCont1.st_nlink = 1;
    tempPtr->statCont1.st_uid = getuid();
    tempPtr->statCont1.st_gid = getgid();
    
    time_t current;
    time(&current);
    tempPtr->statCont1.a_time = current;
    tempPtr->statCont1.m_time = current;
    tempPtr->statCont1.c_time = current;
    
    tempPtr->statCont1.st_size = 0;    
    tempPtr->parentDir = tempPtr2-file;
    tempPtr->childDir = MAXSIZE-1;
    tempPtr->next = MAXSIZE-1;
//  tempPtr->data = NULL;
    tempPtr->type = 2;

    if(tempPtr2->childDir == MAXSIZE-1) {
        tempPtr2->childDir = tempPtr-file;
    } else {
        fsNode *temporary = file+tempPtr2->childDir;
        while(temporary->next != MAXSIZE-1) {
            temporary = file+temporary->next;
        }
        temporary->next = tempPtr-file;
    }
    do_file_write();
    return 0;
}

static int ramd_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){

    if (!check_path(path)) return -ENAMETOOLONG;

    if(memory<size){
        return -ENOSPC;
    }
    fsNode *tempPtr = find_file_node(path);
    size_t curr_size = tempPtr->statCont1.st_size;
    
    if(tempPtr->type == 1) {
        return -EISDIR;
    }
    if(size==0) {
        return size;
    } else if(size>0&&curr_size!=0) {
        if(offset>curr_size) {
            offset=curr_size;
        }
/*
        char *new_file_data = (char *)realloc(tempPtr->data, sizeof(char) * (offset+size));
        if(new_file_data == NULL) {
            return -ENOSPC;
        } else {
*/
//          tempPtr->data = new_file_data;
            memcpy(tempPtr->data + offset, buf, size);
            tempPtr->statCont1.st_size = offset + size;
            time_t current;
            time(&current);
            tempPtr->statCont1.c_time = current;
            tempPtr->statCont1.m_time = current;
            memory = memory+curr_size-(offset + size);
            do_file_write();
            return size;
//      }
    } else if(size>0&&curr_size==0) {
//      tempPtr->data = (char *)malloc(sizeof(char) * size);
        offset = 0;
        memory = memory - size;
        memcpy(tempPtr->data + offset, buf, size);
        tempPtr->statCont1.st_size = offset + size;
        time_t current;
        time(&current);
        tempPtr->statCont1.c_time = current;
        tempPtr->statCont1.m_time = current;
        do_file_write();
        return size;
    }
    return size;
}

static int ramd_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    
    if (!check_path(path)) return -ENAMETOOLONG;
    
    fsNode *tempPtr = find_file_node(path);
    size_t curr_size = tempPtr->statCont1.st_size;
    
    if(tempPtr->type==1)
        return -EISDIR;
    if(offset<curr_size){
        if(offset+size>curr_size) {
            size=curr_size-offset;
        }
        memcpy(buf, tempPtr->data + offset, size);
    } else 
        size = 0;

    if(size > 0) {
        time_t current_time;
        time(&current_time);
        tempPtr->statCont1.a_time = current_time;
    }
    return size;
}

static int ramd_open(const char *path, struct fuse_file_info *fi) {
    
    if (!check_path(path)) return -ENAMETOOLONG;

    if(check_for_path(path)==0)
        return 0;
    else
        return -ENOENT;
}

static int ramd_opendir(const char *path, struct fuse_file_info *fi) {
    
    if (!check_path(path)) return -ENAMETOOLONG;
    return 0;
}

static int ramd_getattr(const char *path, struct stat *stbuf) {
	printf("Before\n");
    if (!check_path(path)) return -ENAMETOOLONG;
	printf("After\n");
    if(check_for_path(path)==0) {
printf("out of check for path\n");
        fsNode *tempPtr = find_file_node(path);
        stbuf->st_nlink = tempPtr->statCont1.st_nlink; 
        stbuf->st_mode  = tempPtr->statCont1.st_mode;
        stbuf->st_uid   = tempPtr->statCont1.st_uid;   
        stbuf->st_gid   = tempPtr->statCont1.st_gid;
        stbuf->st_atime = tempPtr->statCont1.a_time;
        stbuf->st_mtime = tempPtr->statCont1.m_time;
        stbuf->st_ctime = tempPtr->statCont1.c_time; 
        stbuf->st_size = tempPtr->statCont1.st_size;
        return 0;
    }
    return -ENOENT; 
}

static int ramd_utime( const char *path, struct utimbuf *tv ) {
        printf("Inside utime\n");      
        return 0;
}

static int ramd_truncate(const char *path,off_t offset,struct fuse_file_info *fi) {
    return 0;
}

static struct fuse_operations ramd_oper = {
    .open     = ramd_open,
    .read     = ramd_read,
    .write    = ramd_write,
    .create   = ramd_create,
    //.rename   = ramd_rename,
    .mkdir    = ramd_mkdir,
    .unlink   = ramd_unlink,
    .rmdir    = ramd_rmdir,
    .opendir  = ramd_opendir,
    .readdir  = ramd_readdir,
    .getattr  = ramd_getattr,
    .utime = ramd_utime,
    .truncate = ramd_truncate,
};

int main(int argc, char **argv) {

    memory = (long)100000;
    memory = memory * 1024 * 1024; 
	bitmap[0]=1;
    if(FILE_INIT) {
        printf("this is usa\n");
        do_file_write();
        strcpy(fsroot->name, "/");
        time_t current_time;
        time(&current_time);
        fsroot->statCont1.a_time = current_time;
        fsroot->statCont1.m_time = current_time;
        fsroot->statCont1.c_time = current_time;
        
        fsroot->statCont1.st_mode = S_IFDIR | 0755;
        fsroot->statCont1.st_nlink = 2;
        fsroot->statCont1.st_uid = 0;
        fsroot->statCont1.st_gid = 0;

        fsroot->parentDir   = MAXSIZE-1;
        fsroot->childDir     = MAXSIZE-1;
        fsroot->next    = MAXSIZE-1;
        // fsroot->data    = NULL;
        fsroot->type    = 1;
    }
    else {
        printf("this is india\n");
        do_file_read();
    }
    return fuse_main(argc, argv, &ramd_oper, NULL);
}
