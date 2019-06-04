#include "layout.h"


//initializes superblocks and blocks
int superblock_init()
{
	int i;
	superb.totalblocks = TOTAL_BLOCKS;
	superb.totalfreeblocks = TOTAL_BLOCKS;
	for(i=0;i<superb.totalblocks;i++)
	{
		superb.freeblock[i]=i;
	}

	blocks=(struct block **)malloc(sizeof(struct block *)*(superb.totalblocks));
	
	for(i=0;i<superb.totalblocks;i++)
	{
		blocks[i]=(struct block *)malloc(sizeof(struct block));
		blocks[i]->valid=-1;
		blocks[i]->blocknumber=i;
		blocks[i]->data=(void *)malloc(sizeof(char)*BLOCK_SIZE);
		blocks[i]->current_size=0;
		blocks[i]->size=BLOCK_SIZE;
		
	}
	return 0;
}

//given a path, search for the file. If file is not in the root (s = 1) it will search in the directory If s = 0, it returns the directory structure containing that file
int root_search(const char *path, struct file **file_t, struct directory **dir, int s)
{
	//if path is root
	if(strcmp(path,"/")==0)
		return 0;
		
	const char *duplicate_path;
	if(path[0]=='/')
		duplicate_path=path+1;
	else
		duplicate_path=path;
	
	//check if file matches in the root	
	int i;
	for(i=0;i<fsroot->filecount;i++)
	{
		if(strcmp(duplicate_path,fsroot->files[i]->file_name)==0)
		{
			(*file_t)=fsroot->files[i];
			return 1;
		}
	}
	
	//to get the next path
	int namelen = 0;
 	const char *name_end = duplicate_path;
 	while(*name_end != '\0' && *name_end != '/') {
  		  name_end++;
  		  namelen++;
 	 }

	//check for directory
	struct directory * d =(struct directory *)malloc(sizeof(struct directory));
	d = fsroot->dir;
	
	for(i=0;i < fsroot->directorycount ; i++)
	{
		
		if(strncmp(d->directory_name,duplicate_path,namelen)==0)
		{	
			if(s==1)
				return directory_search(d,name_end,file_t);
			else
			{   
				strcpy((*file_t)->file_name,name_end);
				(*dir)=d;
				return 2;
			}
		}
		else
			d=d->next;
			
	}
	return -1;
	
}

//search for the file in the directory
int directory_search(struct directory * d,const char *name,struct file **file_t)
{

	int i;
	if(name[0]=='/')
		name=name+1;
	for(i=0;i<d->filecount;i++)
	{
		if(strcmp(name,d->f[i]->file_name)==0)
		{
			(*file_t)=d->f[i];
			return 1;
		}
	}
	return -1;
}

//Creates a directory
static int do_mkdir(const char *path, mode_t mode)
{
	//can't create / folder
	if(strcmp(path,"/")==0)
		return -1;
	
	//getting the dir name	
	const char *duplicate_path;
	if(path[0]=='/')
		duplicate_path = path+1;
	else
		duplicate_path = path; 
	
	//Initializing the directory
	struct directory *temp=(struct directory *)malloc(sizeof(struct directory));
	
	memset(temp, 0, sizeof(struct directory));
	strcpy(temp->directory_name,duplicate_path);
	temp->directory_id=fsroot->directorycount;
	temp->mode=S_IFDIR | mode;
	temp->filecount=0;
	temp->next=NULL;
	temp->gid=fsroot->gid;
	temp->uid=fsroot->uid;
	
	struct stat *stbuf =(struct stat*)malloc(sizeof(struct stat));
	
	//link dir to data
	if(fsroot->directorycount==0)
	{
		fsroot->dir=(struct directory *)malloc(sizeof(struct directory));
		fsroot->dir=temp;
		fsroot->directorycount=(fsroot->directorycount)+1;
		
		stbuf->st_uid=temp->uid;
		stbuf->st_gid=temp->gid;
		memset(stbuf, 0, sizeof(struct stat));
		stbuf->st_mode  = temp->mode;
		stbuf->st_nlink = 2;
		return 0;
	}
	
	else
	{
		struct directory *t=(struct directory *)malloc(sizeof(struct directory));
		t = fsroot->dir;
		while(t->next!=NULL)
			t=t->next;
		t->next=(struct directory *)malloc(sizeof(struct directory));
		t->next=temp;
		
		fsroot->directorycount=(fsroot->directorycount)+1;
		stbuf->st_uid=temp->uid;
		stbuf->st_gid=temp->gid;
		memset(stbuf, 0, sizeof(struct stat));
		stbuf->st_mode  = temp->mode;
		stbuf->st_nlink = 2;

		return 0;
	}

	return -errno;
}

//Open a file
static int do_open(const char *path, struct fuse_file_info *fi) 
{
	struct file *ft=(struct file *)malloc(sizeof(struct file));
	struct directory *d=(struct directory*)malloc(sizeof(struct directory));
	
	int flag=root_search(path,&ft,&d,1);

	if(flag==1)
	{
		ft->file_count++;
		fi->fh = (uint64_t) ft;
		return 0;
	}
	
		return -ENOENT;
}

//Get file attributes
static int do_getattr(const char *path, struct stat *stbuf,struct fuse_file_info *fi)
{
	
	printf("Entering getattr....\n");
	struct file *ft = (struct file *)malloc(sizeof(struct file));
	struct directory *d = (struct directory*)malloc(sizeof(struct directory));
	stbuf->st_blksize=BLOCK_SIZE;

	int flag = root_search(path, &ft, &d, 1);
	
	printf("%d\n",flag);
	if(flag==0)//root
	{
		  printf("In Root\n");
		  stbuf->st_mode   = fsroot->mode;
		  stbuf->st_uid    = fsroot->uid;
		  stbuf->st_gid    = fsroot->gid;
		  fsroot->last_modified=time(NULL);
		  stbuf->st_atime  = time( NULL ); 
		  stbuf->st_mtime  = time( NULL );
		  fsroot->last_accessed=time(NULL);
		  return 0;
	}
	else if(flag==1)//root files in the root
	{
		  printf("Files in Root\n");
		  stbuf->st_mode   = ft->fil->mode;
		  stbuf->st_size   = ft->fil->size;
		  stbuf->st_blocks = ft->fil->blockcount;
		  stbuf->st_uid    = ft->fil->uid;
		  stbuf->st_gid    = ft->fil->gid;
		  stbuf->st_atime  = time( NULL ); 
		  stbuf->st_mtime  = time( NULL );
		  ft->fil->last_modified=time(NULL);
		  ft->fil->last_accessed=time(NULL);
		  return 0;
	}
	
	else //files within a directory or a root directory
	{
		printf("File in a directory or a directory in the Root\n");
		struct directory *t=(struct directory*)malloc(sizeof(struct directory));
		const char * duplicate_path = path;
		int count=0;

		while(*duplicate_path!='\0')
		{
			if(*duplicate_path=='/')
				count++;
			duplicate_path++;
		}
		if(count==0 || count==1)
		{
			int flag1=root_search(path, &ft, &t, 0);
			if(flag1==2)
			{
				stbuf->st_mode   = t->mode;
				stbuf->st_uid    = t->uid;
				stbuf->st_gid    = t->gid;
				t->last_modified=time(NULL);
		 		stbuf->st_atime  = time( NULL ); 
		 		stbuf->st_mtime  = time( NULL );
		 		t->last_accessed=time(NULL);
				return 0;
			}
	
		else
			 return -ENOENT;
		}
		else
		{
			int flag1=root_search(path, &ft, &t, 0);
			if(flag1==2)
			{
				struct file *f1=(struct file *)malloc(sizeof(struct file));
				int fl=directory_search(t,ft->file_name,&f1);
				if(fl==-1)
					return -ENOENT;
				else
				{
					  stbuf->st_mode   = f1->fil->mode;
					  stbuf->st_size   = f1->fil->size;
					  stbuf->st_blocks = f1->fil->blockcount;
					  stbuf->st_uid    = f1->fil->uid;
					  stbuf->st_gid    = f1->fil->gid;
					  f1->fil->last_modified=time(NULL);
		 			  stbuf->st_atime  = time( NULL ); 
		 			  stbuf->st_mtime  = time( NULL );
		 			  f1->fil->last_accessed=time(NULL);
					return 0;
				}	
			}
		}	
	}
	return 0;
}



//Read Directory
static int do_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
	printf("read dir is called\n");
	
	struct file *ft    = (struct file *)malloc(sizeof(struct file));
	struct directory *d = (struct directory*)malloc(sizeof(struct directory));
	
	int i;
	int flag = root_search(path, &ft, &d, 0);
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	
	if(flag==0)
	{
		for(i=0;i<fsroot->filecount;i++)
			filler(buf, fsroot->files[i]->file_name, NULL, 0);
		
		struct directory *temp=(struct directory*)malloc(sizeof(struct directory));
		temp = fsroot->dir;
		for(i=0;i<fsroot->directorycount;i++)
		{
			filler(buf,temp->directory_name,NULL,0);
			temp=temp->next;
		}
		
	}
	
	else if(flag==2)
	{
		for(i=0;i<d->filecount;i++)
		{
			filler(buf,d->f[i]->file_name,NULL,0);
		}
	}
	
	else
		return -ENOENT;

	return 0;
}

//Create File
int do_create(const char *path, mode_t mode,struct fuse_file_info * fi)
{	
	
	struct file *ft = (struct file *)malloc(sizeof(struct file));
	struct directory *d=(struct directory*)malloc(sizeof(struct directory));
	
	int flag = root_search(path, &ft, &d, 1);

	//exists inside root
	if(flag==1) 
	{	
		printf("File already exists\n");
		return -errno;
	}
	
	else if(flag==-1)
	{
		printf("Creating File..\n");
		
		ft->fil=(struct inode *)malloc(sizeof(struct inode));
		ft->fil->offset=0;
		ft->fil->size=0;
		ft->fil->mode=S_IFREG | 0777;
		ft->fil->blockcount=0;
		ft->fil->block_size=BLOCK_SIZE;
		ft->fil->uid=fsroot->uid;
		ft->fil->gid=fsroot->gid;
		ft->file_count=0;
		
		const char * duplicate_path =path;
		int count=0;

		while(*duplicate_path!='\0')
		{
			if(*duplicate_path=='/')
				count++;
			duplicate_path++;
		}
		if(count==1) //in root
		{

			strcpy(ft->file_name,path+1);
			if(fsroot->filecount==0)
			{
				fsroot->files=(struct file **)malloc(sizeof(struct file *)*10);
			}
		
				fsroot->files[fsroot->filecount]=(struct file*)malloc(sizeof(struct file))	;	
				ft->fil->inode_number=fsroot->filecount;
				fsroot->files[fsroot->filecount]=ft;
				(fsroot->filecount)++;
			
				printf("Creating File..\n");
				return 0;
		}

		else
		{
			root_search(path,&ft,&d,0);
			const char *duplicate_path=path;
			int c=0;
			while(*duplicate_path != '\0')
			{
				if(*duplicate_path=='/')
					c++;
				duplicate_path++;
				if(c==2)
					break;
			}

			strcpy(ft->file_name,duplicate_path);
			if(d->filecount==0)
			{
				d->f=(struct file **)malloc(sizeof(struct file *)*10);
			
			}
		
		
			d->f[d->filecount]=(struct file*)malloc(sizeof(struct file))	;	
			ft->fil->inode_number=d->filecount;
			d->f[d->filecount]=ft;
			(d->filecount)++;
			printf("Creating File..\n");
			return 0;
		
		}
	}		
	printf("File Creation Failed..\n");		
	return -errno;	
}
	
//given a block number and offset, read from the block
int read_block(int blocknum, char *buf, off_t new_off, off_t off, size_t n)
{
	//if blocknum is more than total number of blocks
	if(blocknum > TOTAL_BLOCKS)
		return -1;
		
	//if size asked to read is greater than block size
	if(n > BLOCK_SIZE)
		return -1;	

	//memcpy(destination, source, size)
	memcpy( buf+off, (char *) (blocks[blocknum]->data) + new_off, n);
	return 0;
}

//Reading File
static int read_file(const char *path, char *buf, size_t size,off_t offset, struct fuse_file_info *fi)
{
	struct file *ft=(struct file *)malloc(sizeof(struct file));
	struct directory *d=(struct directory*)malloc(sizeof(struct directory));
	
	int flag=root_search(path,&ft,&d,1);
	
	if(flag==1)
	{
		if(offset > ft->fil->size)
		{
			printf("Can't read. Offset is large\n");
			return 0;
		}
		else
		{
			size_t avail = ft->fil->size - offset; //available size
			
  			size = (size < avail) ? size : avail; //Takes the smaller size
			if(ft->fil->blockcount>0)
			{
				int i;
				for(i=0;i<ft->fil->blockcount;i++)
				{
					if(offset <= ((BLOCK_SIZE-1)*(i+1))) //gives the block index
						break;
				}
				//Calculating offset within the block with the index returned
			off_t new_off=offset-(BLOCK_SIZE * i);	
			size_t new=size;
			off_t off=0;
			avail=BLOCK_SIZE;
			size_t n = (size < avail) ? size : avail; 	

			while(size>0)
			{
				read_block(ft->fil->blocknumber[i],buf,new_off,off, n);
				i=i+1;
				off=off+n;
				size=size-n;
				
				avail=BLOCK_SIZE;
				
				
  				n = (size > avail) ? avail : size;
				new_off=0;	
 			}
				ft->fil->offset=offset+new;
				printf("Read complete..\n");
				return new;
			}
			else 
			{	
				printf("Read Failed\n");
				return 0;
			}
		}
	}
		
	else
	{
		printf("Read Failed\n");
		return -errno;
	}
}

//given a block number and offset, write in the block
int write_block(int blocknum,const char *buf,off_t new_off,off_t off,size_t n)
{
	printf("in writeb %d %ld\n",blocknum,n);
	
	
	if(blocknum > TOTAL_BLOCKS)
		return -1;
		
	if(n > BLOCK_SIZE)
		return -1;
		
		
	memcpy( (blocks[blocknum]->data) +new_off, buf+off, n);
	return 0;
}


static int do_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) 
{
printf("writee is calling ");
	
	struct file *ft=(struct file *)malloc(sizeof(struct file));
	struct directory *d=(struct directory*)malloc(sizeof(struct directory));

	int flag=root_search(path,&ft,&d,1);
	if(flag==1)
	{
		//Calculate the required number of blocks to write
		blkcnt_t req_blocks =( (offset + size + BLOCK_SIZE - 1) / BLOCK_SIZE);
		if(req_blocks<=ft->fil->blockcount)
		{
			int i;
			for(i=0;i<ft->fil->blockcount;i++)
			{
				if(offset <= ((BLOCK_SIZE-1)*(i+1)))
					break;
			}
			
			off_t new_off=offset-(BLOCK_SIZE * i);	
			size_t avail = BLOCK_SIZE-new_off;
  			size_t n = (size > avail) ? avail : size;
			size_t new=size;
			off_t off=0;
			int z;
			while(size>0)
			{
				z=write_block(ft->fil->blocknumber[i],buf,new_off,off, n);
				if(z==-1)
				{	
					printf("No Space");
					return -errno;
				}	
		
				i=i+1;
				off=off+n;
				size=size-n;
				avail = BLOCK_SIZE;
				if(size>0){
				int x=blocks[ft->fil->blocknumber[i]]->current_size;
				blocks[ft->fil->blocknumber[i]]->current_size=x+n;
				}
  				n = (size > avail) ? avail : size;
				
					
				new_off=0;	
 			}
			
			//Updating the file size and offset
			if(ft->fil->size<offset+new)
				ft->fil->size=offset+new;
			ft->fil->offset=offset+size;
			return new;
				
		}

		else
		{
			//No. of extra blocks required
			blkcnt_t remain_b=req_blocks-ft->fil->blockcount;
			int i,x,j;
			for(i=0;i<remain_b;i++)
			{
				x=superb.freeblock[0];
				j=(superb.totalfreeblocks)-1;
				//Take the extra blocks from the freeblocks in superblock
				superb.freeblock[0]= superb.freeblock[j];
			
				(superb.totalfreeblocks)--;
				ft->fil->blocknumber[ft->fil->blockcount]=x;
				(ft->fil->blockcount)++;
					
			}

			for(i=0;i<ft->fil->blockcount;i++)
			{
				if(offset <= ((BLOCK_SIZE)*(i+1)))
					break;
			}
			
			off_t new_off=offset-(BLOCK_SIZE * i);	
			size_t avail = BLOCK_SIZE-new_off;
  			size_t n = (size > avail) ? avail : size;
			size_t new=size;
			off_t off=0;
			int z=0;
			
			
			while(size>0)
			{
				z=write_block(ft->fil->blocknumber[i],buf,new_off,off, n);
				
				if(z==-1)
				{	
					printf("no space");
					return -errno;
				}	
				
				
				i=i+1;
				off=off+n;
			
				size=size-n;
				avail = BLOCK_SIZE;
				
				if(size>0)
				{
				int x=blocks[ft->fil->blocknumber[i]]->current_size;
				blocks[ft->fil->blocknumber[i]]->current_size=x+n;
				}
  				n = (size > avail) ? avail : size;
					
				new_off=0;	
 			}
			
			if(ft->fil->size<offset+new)
				ft->fil->size=offset+new;
			ft->fil->offset=offset+new;
			printf("Write Completed..\n");
			return new;
		}
	
	}		
	else
	{
		printf("Write Failed..\n");
		return -errno;
	}
}


//Remove directory
int do_rmdir(const char *path)
{
    if(path[0] == '/')
    {
        path = path+1;
    }
    struct directory *prev = (struct directory *)malloc(sizeof(struct directory));
    struct directory *tmp = (struct directory *)malloc(sizeof(struct directory));
    
	tmp=fsroot->dir;
	prev=NULL;
	
    while(tmp != NULL)
    {
	if((strcmp(tmp->directory_name,path) == 0)){
	break;
    }
	prev = tmp;
   	tmp = tmp -> next;
    }
    
    if(tmp != NULL)
    {
	    if(tmp->filecount == 0)
	    {
		if(prev!=NULL)
		{
			prev->next = tmp->next;
			fsroot->directorycount = fsroot->directorycount-1;
		}
		else
		{
			fsroot->dir=NULL;
			fsroot->directorycount =fsroot->directorycount- 1;
		}
			printf("Directory removed..\n");
		return 0;

	    }
	}
printf("Cant remove directory. Its not empty\n");
	return -errno;

}

int do_unlink(const char *path)
{
    const char * name=path;
	int count=0;

	while(*name!='\0')
	{
		if(*name=='/')
			count++;
		name++;
	}
	printf("count %d\n",count);
	if(count==0 || count==1) //file is in root
	{

		if(path[0] == '/')
	    {
		path = path+1;
	    }
	    int i;
	    for(i = 0; i < fsroot->filecount ; i++)
	    {
		if(strcmp(fsroot->files[i]->file_name,path)==0)
		    break;
	    }
	    if(i >= fsroot->filecount)
		return -errno;
	    struct inode *fd = fsroot->files[i]->fil;
	    int j;
	    for(j = 0 ; j < fd->blockcount; j++)
	    {
		superb.freeblock[superb.totalfreeblocks++]=fd->blocknumber[j]; //freeing the blocks allocated to the file
		blocks[fd->blocknumber[j]]->current_size = 0;
	    }
	    if(i == fsroot->filecount-1){
		fsroot->filecount = fsroot->filecount-1;
		printf("File Deleted..\n");
		return 0;
	}
	    else
		{
		    fsroot->files[i] = fsroot->files[fsroot->filecount-1];
		    fsroot->filecount = fsroot->filecount-1;
		    printf("File Deleted..\n");
		    return 0;
		}
		printf("File Deletion failed..\n");
	    return -errno;
	}

	else //file is in a directory
	{
		
		struct file *ft=(struct file *)malloc(sizeof(struct file));
		struct directory *d=(struct directory*)malloc(sizeof(struct directory));
		int flag=root_search(path,&ft,&d,0);

		if(flag!=2) 
		{
			printf("File Deletion failed..\n");
			return -errno;
		}
		
		 int i;
		const char *p=ft->file_name;
		if(p[0]=='/')
			p=p+1;
	    for(i = 0; i < d->filecount ; i++)
	    {
		if(strcmp(d->f[i]->file_name,p)==0)
		    break;
	    }
	    if(i >= d->filecount)
		return -errno;
	    struct inode *fd = d->f[i]->fil;
	    int j;
	    for(j = 0 ; j < fd->blockcount; j++)
	    {
		superb.freeblock[superb.totalfreeblocks++]=fd->blocknumber[j];
		blocks[fd->blocknumber[j]]->current_size = 0;
	    }
	    if(i == d->filecount-1){
		d->filecount = d->filecount-1;
		printf("File Deleted..\n");
		return 0;}
	    else
		{
		    d->f[i] = d->f[d->filecount-1];
		    d->filecount = d->filecount-1;
		    printf("File Deleted..\n");
		    return 0;
		}
		printf("File Deletion Failed..\n");
	    return -errno;
				
	}

    }



void do_destroy(void *private_data)
{
	printf("file destroy\n");
	FILE *d;
	int i,j;
	//Binary file
	d=fopen("/home/mukesh/Desktop/osfinal/dir.txt","wb+");
	//Initializing the data structure
	fseek(d,0,SEEK_SET);
	fwrite(fsroot,sizeof(*fsroot),1,d);
		
	struct directory *temp_d;
	temp_d=fsroot->dir;
	
	struct file *temp_f;
	struct inode *temp_fi;
	struct block *temp_b;
	//write root directories and files within them
	for(i=0;i<fsroot->directorycount;i++)
	{
		fwrite(temp_d,sizeof(*temp_d),1,d);
		
		for(i=0;i<temp_d->filecount;i++)
		{
			
			temp_f=temp_d->f[i];
			fwrite(temp_f,sizeof(*temp_f),1,d);
			temp_fi=temp_f->fil;
			fwrite(temp_fi,sizeof(*temp_fi),1,d);
			for(j=0;j<temp_fi->blockcount;j++)
			{
				void *temp_da;
				temp_b=blocks[temp_fi->blocknumber[j]];
				fwrite(temp_b,sizeof(*temp_b),1,d);
				temp_da=temp_b->data;
				fwrite(temp_da,sizeof(*temp_da),BLOCK_SIZE,d);
				free(temp_da);
				
			
			}
		}


		temp_d=temp_d->next;
	}		
	
	//write the superblocks
	fwrite(&superb,sizeof(superb),1,d);
	
	
	//write only root files
	for(i=0;i<fsroot->filecount;i++)
	{
		
		temp_f=fsroot->files[i];
		fwrite(temp_f,sizeof(*temp_f),1,d);
		temp_fi=temp_f->fil;
		fwrite(temp_fi,sizeof(*temp_fi),1,d);
		for(j=0;j<temp_fi->blockcount;j++)
		{
			void *temp_da;
			temp_b=blocks[temp_fi->blocknumber[j]];
			fwrite(temp_b,sizeof(*temp_b),1,d);
			temp_da=temp_b->data;
			fwrite(temp_da,sizeof(*temp_da),BLOCK_SIZE,d);
			free(temp_da);
			
		}
	}

	
	fclose(d);
	printf("Write to Binary File Complete..\n");
}
	

int persistent()
{
	FILE *d;
	int i,j;
	
	d=fopen("/home/mukesh/Desktop/osfinal/dir.txt","rb+");
	if(d)
		printf("persistent\n");
	else
		printf("not persistent\n");
	fseek(d,0,SEEK_END);
	printf("1\n");
	long int p=ftell(d);
	printf("2\n");
	if(p==0)
	{
		fclose(d);
		return 0;
	}
	printf("Restoring original File System..\n");
	fseek(d,0,SEEK_SET);
	fsroot=(struct root *)malloc(sizeof(struct root));
	memset(fsroot, 0, sizeof(struct root));
	
	fread(fsroot,sizeof(*fsroot),1,d);
	
	struct directory *prev;
	
	//read root directories and files within them
	for(i=0;i<fsroot->directorycount;i++)
	{
		struct directory *temp_d=(struct directory*)malloc(sizeof(struct directory));
		fread(temp_d,sizeof(*temp_d),1,d);
		if(i==0)
		{
			temp_d->next=NULL;
			fsroot->dir=temp_d;
			prev=temp_d;
			
		}
		else
		{
			
			temp_d->next=NULL;
			prev->next=temp_d;
			prev=temp_d;
		
		}

		temp_d->f=(struct file **)malloc(sizeof(struct file *)*10);
		for(i=0;i<temp_d->filecount;i++)
		{
		
			struct file *temp_f=(struct file*)malloc(sizeof(struct file));
			struct inode *temp_fi=(struct inode*)malloc(sizeof(struct inode));
				
			fread(temp_f,sizeof(*temp_f),1,d);
			fread(temp_fi,sizeof(*temp_fi),1,d);
		
			for(j=0;j<temp_fi->blockcount;j++)
			{
			
				struct block *temp_b=(struct block*)malloc(sizeof(struct block));
				void *temp_da=(void *)malloc(sizeof(char)*BLOCK_SIZE);
				//printf("in b %d\n",temp_fi->blocknumber[j]);
			
				fread(temp_b,sizeof(*temp_b),1,d);
				fread(temp_da,sizeof(*temp_da),BLOCK_SIZE,d);
			
				temp_b->data=(char *)temp_da;
				blocks[temp_fi->blocknumber[j]]=temp_b;
			}
		
			temp_f->fil=temp_fi;
			temp_d->f[i]=temp_f;
		
		}
	
		
	}		
	
	//reading superblocks
	fread(&superb,sizeof(superb),1,d);
	
	//reading root files
	fsroot->files=(struct file **)malloc(sizeof(struct file *)*10);
	for(i=0;i<fsroot->filecount;i++)
	{
		
		struct file *temp_f=(struct file*)malloc(sizeof(struct file));
		struct inode *temp_fi=(struct inode*)malloc(sizeof(struct inode));
				
		fread(temp_f,sizeof(*temp_f),1,d);
		fread(temp_fi,sizeof(*temp_fi),1,d);
		
		for(j=0;j<temp_fi->blockcount;j++)
		{
			
			struct block *temp_b=(struct block*)malloc(sizeof(struct block));
			void *temp_da=(void *)malloc(sizeof(char)*BLOCK_SIZE);
			//printf("in b %d\n",temp_fi->blocknumber[j]);
			
			fread(temp_b,sizeof(*temp_b),1,d);
			fread(temp_da,sizeof(*temp_da),BLOCK_SIZE,d);
			
			temp_b->data=(char *)temp_da;
			blocks[temp_fi->blocknumber[j]]=temp_b;
		}
		
		temp_f->fil=temp_fi;
		fsroot->files[i]=temp_f;
		
	}
	printf("File System Restored..\n");
	fclose(d);
	return 1;

}

static struct fuse_operations file_oper = {
  .getattr      = do_getattr,
  .readdir      = do_readdir,
  .mkdir        = do_mkdir,
  .open         = do_open,
  .read         = read_file,
  .write        = do_write,
  .create    	= do_create,
  .rmdir 	= do_rmdir,
  .unlink	= do_unlink,
  .destroy	= do_destroy,
};

int main(int argc, char *argv[]) 
{
	umask(0);
	superblock_init();
	if(persistent()==0)
	{
	printf("inside\n");
	fsroot=(struct root *)malloc(sizeof(struct root));
	memset(fsroot, 0, sizeof(struct root));
	
	fsroot->uid=getuid();
	fsroot->gid=getgid();
	fsroot->mode=S_IFDIR | 0777;
	fsroot->filecount=0;
	fsroot->directorycount=0;
	fsroot->dir=NULL;
	}
	//persistent();
	return fuse_main(argc, argv, &file_oper, NULL);
}
