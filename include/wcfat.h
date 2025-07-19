// wcfat - gom Wing's Compact Libray for FAT32 filesystem
// Copyright (c) 2023-2024, gom Wing <
//
// This software is released under the MIT License.
// https://opensource.org/license/mit/
// wcfat.h - Header file for wcfat library

#ifndef WCFAT_H
#define WCFAT_H

typedef unsigned char		uint8;		typedef unsigned short		uint16;		typedef unsigned long		uint32;		typedef unsigned long long	uint64;
typedef	char				int8;		typedef	short				int16;		typedef	long				int32;		typedef	long long			int64;

#ifdef __GNUC__
#define __packed	__attribute__((packed))	
#endif

//------------------------------------------------------------------------------
//

// Structures for FAT32 filesystem
typedef struct __packed {
	uint8	bootflag, chs_start[3], type, chs_end[3];
	uint32	lba, size;
} PartEntry;

typedef struct __packed {
	uint8		code[446];
	PartEntry	part[4];
	uint16		sig;
} MBR;

typedef struct __packed {
	uint8	jump[3];
	char	name[8];
	uint16	bytes_per_sect;
	uint8	sect_per_clust;
	uint16	res_sect_cnt;
	uint8	fat_cnt;
	uint16	root_ent_cnt;
	uint16	sect_cnt_16;
	uint8	media;
	uint16	sect_per_fat_16;
	uint16	sect_per_track;
	uint16	head_cnt;
	uint32	hidden_sect_cnt;
	uint32	sect_cnt_32;
	uint32	sect_per_fat_32;
	uint16	ext_flags;
	uint8	minor;
	uint8	major;
	uint32	root_cluster;
	uint16	info_sect;
	uint16	copy_bpb_sector;
	uint8	reserved_0[12];
	uint8	drive_num;
	uint8	reserved_1;
	uint8	boot_sig;
	uint32	volume_id;
	char	volume_label[11];
	char	fs_type[8];
	uint8	reserved_2[420];
} Bpb510;

typedef struct __packed {
	uint8	code[12];
	uint8	bps;  // bytes per sector * 256
	uint8	spc;
	uint16	cnt_rsv;
	
	//uint8	cnt_fat;
	//uint8	must_zero[4];
	//uint8	media;
	//uint16	fatsz_16;
	uint32	cnt_fat;
	uint32	media_type;
	uint16	sect_per_track;
	uint16	cnt_head;
	uint32	cnt_hidden;
	
	uint32	sect_cnt_32;
	uint32	fatsz_32;
	uint16	ext_flags;
	uint16	fs_version;
	uint32	root_cluster;

	uint16	info_sect;
	uint16	copy_bpb_sector;
	uint8	reserved_0[12];
	uint8	drive_num;
	uint8	reserved_1;
	uint8	boot_sig;
	uint32	volume_id;
	char	volume_label[11];
	char	fs_type[8];
	uint8	reserved_2[418];
	uint16		sig;
} Bpb710;

//typedef union __packed {
//	uint8	raw[32];
//	struct {
//		uint8	seq;
//		uint8	name0[10];
//		uint8	attr;
//		uint8	type;
//		uint8	crc;
//		uint8	name1[12];
//		uint16	clust;
//		uint8	name2[4];
//	};
//} Lfn;

typedef union {
	struct {
		unsigned short day	: 5; // Day of the month (1-31)
		unsigned short month: 4; // Month (1-12)
		unsigned short year	: 7; // Year since 1980 (0-127, representing 1980-2107)
	};
	unsigned short data; // Combined date field
} fatdate;

typedef union {
	struct {
		unsigned short sec	: 5; // sec / 2 = fat_sec
		unsigned short min	: 6; // minute (1-12)
		unsigned short hour : 5; // 0-24
	};
	unsigned short data; // Combined date field
} fattime;

typedef struct __packed {
	uint8	name[11];
	uint8	attr;
	uint8	type;
	uint8	tenth;
	fattime	crttime;
	fatdate	crtdate;
	fatdate	accdate;
	uint16	clust_hi;
	fattime	wrttime;
	fatdate	wrtdate;
	uint16	clust_lo;
	uint32	size;
} Sfn;

typedef union __packed {
	uint8	raw[32];
	struct {
		uint8	order;
		uint8	name1[10];
		uint8	attr;
		uint8	type;
		uint8	checksum;
		uint8	name2[12];
		uint16	clust;
		uint8	name3[4];
	};
} Lfn;

typedef struct __packed {
	uint32	head_sig;
	uint8	reserved_0[480];
	uint32	struct_sig;
	uint32	free_cnt;
	uint32	next_free;
	uint8	reserved_1[12];
	uint32	tail_sig;
} FsInfo;

typedef struct {
#if 0
	const DISC_INTERFACE* disc;
	CACHE* cache;
	// Info about the partition
	FS_TYPE               filesysType;
	uint64_t              totalSize;
	sec_t                 rootDirStart;
	uint32_t              rootDirCluster;
	uint32_t              numberOfSectors;
	sec_t                 dataStart;
	uint32_t              bytesPerSector;
	uint32_t              sectorsPerCluster;
	uint32_t              bytesPerCluster;
	uint32_t              fsInfoSector;
	FAT                   fat;
	// Values that may change after construction
	uint32_t              cwdCluster;			// Current working directory cluster
	int                   openFileCount;
	struct _FILE_STRUCT* firstOpenFile;		// The start of a linked list of files
	mutex_t               lock;					// A lock for partition operations
	bool                  readOnly;
#endif	// If this is set, then do not try writing to the disc
	uint32            lba;
	uint32            root_cluster;
	uint32			fat_sect[2];			// LBA of the two FAT tables
	uint32            size;
	uint32			clust_shift;			// Shift for cluster size

	uint32			total_sect;
	uint32			total_clust;
	uint32			info_sect;				// LBA of the FsInfo sector
	uint32			data_sect;				// LBA of the data area
	uint32			last_used;				// Last used cluster
	uint32			free_cnt;				// Number of free clusters

	uint32			current_cluster;		// Current cluster for operations`
	uint32			sect;					// Current sector for operations
	// Flags for the partition (e.g., mounted, dirty)
	
	char                  label[12];			// Volume label

} PartInfo;

typedef struct {
	uint32		size;
	uint32		first_cluster;		// Shift for cluster size
	uint32		chain_info[120];
} FileInfo;

int probe(/*DiskOps* ops, int partition, */void** lba);

#endif // WCFAT_H
	
	
	
