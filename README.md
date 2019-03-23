# AFS
A File System
PHASE 1- SYSTEM CALL IMPLEMENTATION

Installed FUSE and ran sample programs by mounting and running
Understood why FUSE is like a daemon for handling the system calls
Basically FUSE makes it easire for us to implement a file system 
	As it handles the system call linking
	And makes mounting easier
Basic operations were implemented such as mkdir, open, create, readdir, mkdir etc
Chalked out and coded the memory management for the FS
Made the basic structure to the File System

INDIVIDUAL FUNCTIONS

ramd_opendir -> Checks for the pathname and opens the dir if present
ramd_open -> Checks the pathname and opens the given file by if present
ramd_read -> takes the given path of the file and copies the file contents into a buffer if file is present
ramd_write -> checks the path and 
ramd_create ->
ramd_rmdir ->
ramd_readdir ->
ramd_mkdir ->
ramd_unlink ->
find_file_node ->
check_for_path -> Checks whether the path already exists
check_path -> Will see whether the path is valid or not
find_free_index ->


PHASE 2- 
Went more into details of the FS Storage, Data Abstraction
Below are the files for a single inode in the structure and the metadata for that node

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

};

typedef struct fileNode
{
    int    parentDir;
    int childDir;
    int next;
    char    name[255];
    struct mystat statCont1;
    char    data[500];
    int     type; //0 - default/1 - dir/2 - file
};

As is evident, we have used a tree data structure that is similar to a linked list converted tree
An inode contains the following
parentDir -> The address to its parent. root will have NULL
childDir -> The directory present in the current directory
next -> Used to move to the next file/dir in the same dir

statCont1 -> metadata of the file
data -> core data which the file is to store
type -> 0 as default 
	1 for a directory
	2 for a file


PHASE 3- SECONDARY STORAGE
Implemented Persistence and Secondary Storage by dumping in all FS contents into a binary file during the time of exit
When needed the entire FS structure is loaded from the binary file into memory
So the files remain intact and can be accessed after mount with data content preserved


FEATURES
Easy traversing through the tree structure
	Firstly due to the ease of handling trees
	Secondly the pointers such as child, parent and next make it easier compared to the usual child
	This decreases the backtracking cost
