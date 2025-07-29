#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>

#include "../source/common.h"

typedef unsigned int sec_t;
typedef unsigned int u32;
typedef			 int s32;
typedef unsigned char u8;
typedef unsigned short u16;

typedef bool (*FN_MEDIUM_STARTUP)(void);
typedef bool (*FN_MEDIUM_ISINSERTED)(void);
typedef bool (*FN_MEDIUM_READSECTORS)(sec_t sector, sec_t numSectors, void* buffer);
typedef bool (*FN_MEDIUM_WRITESECTORS)(sec_t sector, sec_t numSectors, const void* buffer);
typedef bool (*FN_MEDIUM_CLEARSTATUS)(void);
typedef bool (*FN_MEDIUM_SHUTDOWN)(void);

struct DISC_INTERFACE_STRUCT {
	unsigned long			ioType;
	unsigned long			features;
	FN_MEDIUM_STARTUP		startup;
	FN_MEDIUM_ISINSERTED	isInserted;
	FN_MEDIUM_READSECTORS	readSectors;
	FN_MEDIUM_WRITESECTORS	writeSectors;
	FN_MEDIUM_CLEARSTATUS	clearStatus;
	FN_MEDIUM_SHUTDOWN		shutdown;
};
typedef struct DISC_INTERFACE_STRUCT DISC_INTERFACE;

bool dldiStartup(void) 
{
	// This function should initialize the DLDI interface, but is not implemented here.
	return true; // Assuming initialization is successful for this example.
}

FILE* fp;

bool dldiIsInserted(void) 
{
	fp = fopen("ndstflash.img", "rb");
		
	// This function should check if the DLDI medium is inserted, but is not implemented here.
	//return true; // Assuming the medium is always inserted for this example.
	return (fp!=NULL);
}

bool dldiReadSectors(sec_t sector, sec_t numSectors, void* buffer) 
{
	unsigned long long offset = sector * 512; // Assuming sector size is 512 bytes 
	// This function should read sectors from the DLDI medium, but is not implemented here.
	
	fsetpos(fp, (fpos_t*)&offset);	//fseek(fp, offset, SEEK_SET);
	//fsetpos64(fp, (fpos_t*)&offset);	//fseek(fp, offset, SEEK_SET);

	uint32_t ret=fread(buffer, 512, numSectors, fp);
	return true; // Assuming read operation is successful for this example.
}

bool dldiWriteSectors(sec_t sector, sec_t numSectors, const void* buffer) 
{
	// This function should write sectors to the DLDI medium, but is not implemented here.
	return true; // Assuming write operation is successful for this example.
}

bool dldiClearStatus(void) 
{
	// This function should clear the status of the DLDI medium, but is not implemented here.
	return true; // Assuming status clear operation is successful for this example.
}

bool dldiShutdown(void) 
{
	// This function should shut down the DLDI medium, but is not implemented here.
	return true; // Assuming shutdown operation is successful for this example.
}

DISC_INTERFACE dldi = {
	0, // ioType
	0, // features
	dldiStartup, // startup function
	dldiIsInserted, // isInserted function
	dldiReadSectors, // readSectors function
	dldiWriteSectors, // writeSectors function
	dldiClearStatus, // clearStatus function
	dldiShutdown // shutdown function
};

const DISC_INTERFACE* get_io_dsisd(void) 
{
	return NULL; // This function should return a pointer to a DISC_INTERFACE structure for DSISD, but is not implemented here.
}

const DISC_INTERFACE* dldiGetInternal(void) 
{
	return &dldi;
}

struct _reent
{
	int _errno;
	// Other members are not used in this file, so they are not defined here
};

//typedef struct {
//	int device;
//	void* dirStruct;
//} DIR_ITER;
struct statvfs {
	unsigned long f_bsize;    /* file system block size */
	unsigned long f_frsize;   /* fragment size */
	unsigned long f_blocks;   /* size of fs in f_frsize units */
	unsigned long f_bfree;    /* free blocks in fs in f_frsize units */
	unsigned long f_bavail;   /* free blocks avail to non-superuser in f_frsize units */
	unsigned long f_files;    /* total file nodes in file system */
	unsigned long f_ffree;    /* free file nodes in fs */
	unsigned long f_favail;   /* free file nodes avail to non-superuser */
	unsigned long f_fsid;     /* file system ID */
	unsigned long f_flag;     /* mount flags */
	unsigned long f_namemax;  /* maximum filename length */
};


typedef long off32_t;
typedef off32_t off_t;

struct timeval
{
	long tv_sec;
	long tv_usec;
};

typedef struct {
	const char* name;
	size_t structSize;
	int (*open_r)(struct _reent* r, void* fileStruct, const char* path, int flags, int mode);
	int (*close_r)(struct _reent* r, void* fd);
	ssize_t(*write_r)(struct _reent* r, void* fd, const char* ptr, size_t len);
	ssize_t(*read_r)(struct _reent* r, void* fd, char* ptr, size_t len);
	off_t(*seek_r)(struct _reent* r, void* fd, off_t pos, int dir);
	int (*fstat_r)(struct _reent* r, void* fd, struct stat* st);
	int (*stat_r)(struct _reent* r, const char* file, struct stat* st);
	int (*link_r)(struct _reent* r, const char* existing, const char* newLink);
	int (*unlink_r)(struct _reent* r, const char* name);
	int (*chdir_r)(struct _reent* r, const char* name);
	int (*rename_r) (struct _reent* r, const char* oldName, const char* newName);
	int (*mkdir_r) (struct _reent* r, const char* path, int mode);

	size_t dirStateSize;

	DIR_ITER* (*diropen_r)(struct _reent* r, DIR_ITER* dirState, const char* path);
	int (*dirreset_r)(struct _reent* r, DIR_ITER* dirState);
	int (*dirnext_r)(struct _reent* r, DIR_ITER* dirState, char* filename, struct stat* filestat);
	int (*dirclose_r)(struct _reent* r, DIR_ITER* dirState);
	int (*statvfs_r)(struct _reent* r, const char* path, struct statvfs* buf);
	int (*ftruncate_r)(struct _reent* r, void* fd, off_t len);
	int (*fsync_r)(struct _reent* r, void* fd);

	void* deviceData;

	int (*chmod_r)(struct _reent* r, const char* path, mode_t mode);
	int (*fchmod_r)(struct _reent* r, void* fd, mode_t mode);
	int (*rmdir_r)(struct _reent* r, const char* name);
	int (*lstat_r)(struct _reent* r, const char* file, struct stat* st);
	int (*utimes_r)(struct _reent* r, const char* filename, const struct timeval times[2]);

	long (*fpathconf_r)(struct _reent* r, int fd, int name);
	long (*pathconf_r)(struct _reent* r, const char* path, int name);

	int (*symlink_r)(struct _reent* r, const char* target, const char* linkpath);
	ssize_t(*readlink_r)(struct _reent* r, const char* path, char* buf, size_t bufsiz);

} devoptab_t;


DIR_ITER* _WC_diropen_r(struct _reent* r, DIR_ITER* dirState, const char* path);
int _WC_open_r(struct _reent* r, void* fileStruct, const char* path, int flags, int mode);
int _WC_close_r(struct _reent* r, void* fd);
ssize_t _WC_write_r(struct _reent* r, void* fd, const char* ptr, size_t len);
ssize_t _WC_read_r(struct _reent* r, void* fd, char* ptr, size_t len);

#if 0
int AddDevice(const devoptab_t* device) {}
int FindDevice(const char* name) {}
int RemoveDevice(const char* name) {}
void setDefaultDevice(int device) {}
const devoptab_t* GetDeviceOpTab(const char* name) {}
#endif

#include "wcfat.h"

char buffer[65536*4]; // Buffer for reading sectors

struct _FILE_STRUCT {
	uint32_t             ptr;
	uint32_t             mode;
	uint32_t             filesize;
	uint32_t			 chainCount;
	uint32_t             startCluster;
	uint32_t             cluster_chain_buffer[(512 / 4 - 5)/2];
};

typedef struct _FILE_STRUCT FILE_STRUCT;

int main(void) {
#if 0
    if (!fatInitDefault()) {
        printf("Fat init error !!!\n");
        //while (1)
        //    swi::IntrWait(1, IRQ_VBLANK); //swiWaitForVBlank();
        //return 1;
    }
#else
	void *lba;
	if (!probe(&lba)) {
		printf("Fat init error !!!\n");
		//while (1)
		//    swi::IntrWait(1, IRQ_VBLANK); //swiWaitForVBlank();
		//return 1;
	}
	struct _reent r;

#if 1
	FILE_STRUCT fileStruct;
	//int ret = _WC_open_r(&r, &fileStruct, "fat:/[_GP2X_]/game/PicoDrive/readme.txt",0,0);
	int ret = _WC_open_r(NULL, &fileStruct, "fat:/PCE-cd/ys4.bin", 0, 0);
	//_WC_open_r(NULL, &fileStruct, "fat:/_에뮬/nand/학습/test.mp3", 0, 0);

	if (ret) {
		printf("File opened successfully!\n");
		printf("File size: %u bytes\n", fileStruct.filesize);
		printf("Start cluster: %u\n", fileStruct.startCluster);
		printf("Chain count: %u\n", fileStruct.chainCount);
	
		_WC_read_r(&r, &fileStruct, buffer, 123);
		_WC_read_r(&r, &fileStruct, buffer+123, 45);
		_WC_read_r(&r, &fileStruct, buffer+123+45, 32768);
		_WC_read_r(&r, &fileStruct, buffer + 123 + 45+ 32768, 4097);
		_WC_read_r(&r, &fileStruct, buffer + 123 + 45 + 32768+4097, 40970);



	} else 
		printf("Failed to open file.\n");
#endif
	DIR_ITER dirIter;
	_WC_diropen_r(&r, &dirIter, "fat:/[_GP2X_]/game/../game/PicoDrive/brm");
	_WC_diropen_r(&r, &dirIter, "fat:/[_GP2X_]/game/PicoDrive/brm/");
	_WC_diropen_r(&r, &dirIter, "fat:/[_GP2X_]/game/PicoDrive/br2/");
	_WC_diropen_r(&r, &dirIter, "fat:/[_GP2X_]/game/PicoDrive/br2");
	_WC_diropen_r(&r, &dirIter, "fat:/[_GP2X_]/game/PicoDrivo/brm");
	_WC_diropen_r(&r, &dirIter, "fat:/[_GP2X_]/game/PicoDrivo/brm/");




#endif
    //#endif
}