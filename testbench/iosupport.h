//---------------------------------------------------------------------------------
#ifndef __iosupp_h__
#define __iosupp_h__
//---------------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

//#include <reent.h>
#include <sys/stat.h>
//#include <sys/statvfs.h>

	enum {
		STD_IN,
		STD_OUT,
		STD_ERR,
		STD_MAX = 16
	};


	typedef struct {
		int device;
		void* fileStruct;
	} __handle;

	/* Directory iterator for mantaining state between dir* calls */
	typedef struct {
		int device;
		void* dirStruct;
	} DIR_ITER;

	typedef struct {
		const char* name;
		int	structSize;
		int (*open_r)(struct _reent* r, void* fileStruct, const char* path, int flags, int mode);
		int (*close_r)(struct _reent* r, int fd);
		ssize_t(*write_r)(struct _reent* r, int fd, const char* ptr, size_t len);
		ssize_t(*read_r)(struct _reent* r, int fd, char* ptr, size_t len);
		off_t(*seek_r)(struct _reent* r, int fd, off_t pos, int dir);
		int (*fstat_r)(struct _reent* r, int fd, struct stat* st);
		int (*stat_r)(struct _reent* r, const char* file, struct stat* st);
		int (*link_r)(struct _reent* r, const char* existing, const char* newLink);
		int (*unlink_r)(struct _reent* r, const char* name);
		int (*chdir_r)(struct _reent* r, const char* name);
		int (*rename_r) (struct _reent* r, const char* oldName, const char* newName);
		int (*mkdir_r) (struct _reent* r, const char* path, int mode);

		int dirStateSize;

		DIR_ITER* (*diropen_r)(struct _reent* r, DIR_ITER* dirState, const char* path);
		int (*dirreset_r)(struct _reent* r, DIR_ITER* dirState);
		int (*dirnext_r)(struct _reent* r, DIR_ITER* dirState, char* filename, struct stat* filestat);
		int (*dirclose_r)(struct _reent* r, DIR_ITER* dirState);
		int (*statvfs_r)(struct _reent* r, const char* path, struct statvfs* buf);
		int (*ftruncate_r)(struct _reent* r, int fd, off_t len);
		int (*fsync_r)(struct _reent* r, int fd);

		void* deviceData;

		int (*chmod_r)(struct _reent* r, const char* path, mode_t mode);
		int (*fchmod_r)(struct _reent* r, int fd, mode_t mode);

	} devoptab_t;

	extern const devoptab_t* devoptab_list[];

	//typedef struct {
	//	void* (*sbrk_r) (struct _reent* ptr, ptrdiff_t incr);
	//	int (*lock_init) (int* lock, int recursive);
	//	int (*lock_close) (int* lock);
	//	int (*lock_release) (int* lock);
	//	int (*lock_acquire) (int* lock);
	//	void (*malloc_lock) (struct _reent* ptr);
	//	void (*malloc_unlock) (struct _reent* ptr);
	//	void (*exit) (int rc);
	//	int (*gettod_r) (struct _reent* ptr, struct timeval* tp, struct timezone* tz);
	//} __syscalls_t;

	//extern __syscalls_t __syscalls;

	int AddDevice(const devoptab_t* device);
	int FindDevice(const char* name);
	int RemoveDevice(const char* name);
	void setDefaultDevice(int device);
	const devoptab_t* GetDeviceOpTab(const char* name);

#ifdef __cplusplus
}
#endif

//---------------------------------------------------------------------------------
#endif // __iosupp_h__
//---------------------------------------------------------------------------------