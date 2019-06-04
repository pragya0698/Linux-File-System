#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>
#include <time.h>
#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#define BLOCK_SIZE 512
#define TOTAL_BLOCKS 15


struct superblock
{
	int totalblocks;	
	int totalfreeblocks;
	int freeblock[TOTAL_BLOCKS];
};

struct inode
{
	int inode_number;
	off_t offset;
	size_t size;
	mode_t mode;
	time_t last_modified;
	time_t last_accessed;
	blkcnt_t blockcount;
	blksize_t block_size;
	int blocknumber[8];
	int uid;
	int gid;
};

struct file
{
	char file_name[50];	
	struct inode *fil;
	unsigned int file_count;
};
	
struct directory
{
	char directory_name[50];
	int directory_id;
	mode_t mode;
	struct file **f;
	int filecount;
	struct directory *next;
	time_t last_modified;
	time_t last_accessed;
	int uid;
	int gid;
};

struct block
{
	int blocknumber;
	int valid;
	void *data;
	size_t size;
	size_t current_size;
};


struct root
{
	struct directory *dir;
	struct file **files;
	int filecount;
	int directorycount;
	int uid;
	int gid;
	time_t last_modified;
	time_t last_accessed;
	mode_t mode;
};

struct block **blocks;	
struct superblock superb;		
struct root *fsroot;

int root_search(const char *path,struct file **file_t,struct directory **dir,int s);
int directory_search(struct directory *d,const char *name,struct file **file_t);
int intialize();

int read_block(int blocknr, char *buf,off_t new_off,off_t off,size_t n);
int write_block(int blocknr,const char *buf,off_t new_off,off_t off,size_t n);
