// wcfat - gom Wing's Compact Libray for FAT32 filesystem
// Copyright (c) 2023-2024, gom Wing <
//
// This software is released under the MIT License.
// https://opensource.org/license/mit/
// wcfat.h - Header file for wcfat library

#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include "../source/common.h"
#include "wcfat.h"

typedef unsigned int sec_t;

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

#define FEATURE_MEDIUM_CANREAD      0x00000001
#define FEATURE_MEDIUM_CANWRITE     0x00000002

DISC_INTERFACE* disc = NULL;

#define strncmp wc_strncmp
#define memcpy  wc_memcpy
#define memset  wc_memset
#define min     wc_min


#define wc_min(x,y) ((x<y)?(x):(y))  

//------------------------------------------------------------------------------
static int wc_strncmp(const char* _Str1, const char *_Str2, size_t _MaxCount)
{
    //if (_MaxCount == 0) return 0;
    do {
        if (*_Str1 != *_Str2++) return (*(char const*)_Str1 - *(char const*)--_Str2);
        if (*_Str1++ == 0) break;
    } while (--_MaxCount != 0);
    return 0;
}

static uint32 wc_toupper(const wchar_t c)
{
	if (c < 'a' || c > 'z') return (uint32)c; 
    else return c - ('a' - 'A');
}

static void* wc_memcpy(void* _Dst, void const* _Src, size_t _Size) {
    char* dest = _Dst, * Dst = _Dst; char* Src = (char*)_Src;
    while (_Size-- > 0) *Dst++ = *Src++;
    return dest;
}

static void* wc_memset(void* _Dst, int _Val, size_t _Size)
{
    char* ptr = (char*)_Dst;
    do {
        *ptr++ = _Val;
    } while (--_Size != 0);
    return _Dst;
}

static int wc_wstrcasecmp(const wchar_t* str1, const wchar_t* str2)
{
    while (*str1 && *str2 && (wc_toupper(*str1) == wc_toupper(*str2))) {
        str1++;
        str2++;
    }
    return wc_toupper(*str1) - wc_toupper(*str2);
}

#define EXT_FLAG_MIRROR  (1 << 7)
#define EXT_FLAG_ACT     0x000f

enum
{
    FAT_ERR_NONE = 0,
    FAT_ERR_NOFAT = -1,
    FAT_ERR_BROKEN = -2,
    FAT_ERR_IO = -3,
    FAT_ERR_PARAM = -4,
    FAT_ERR_PATH = -5,
    FAT_ERR_EOF = -6,
    FAT_ERR_DENIED = -7,
    FAT_ERR_FULL = -8,
};

PartInfo _pi = { 0 };

static bool check_fat(uint8 *buf, uint32 lba)
{
    Bpb710* bpb = (Bpb710*)buf;

    //if (bpb->jump[0] != 0xeb && bpb->jump[0] != 0xe9)
    //    return false;

    // Check if we need to be this strict.
    //if (bpb->fat_cnt != 2)
    //    return false;

    if (bpb->cnt_fat>2 || bpb->media_type !=0xf800) return false;
    if (bpb->info_sect != 1)                        return false;
    if (strncmp(bpb->fs_type, "FAT32   ", 8))        return false;
    //if (bpb->bytes_per_sect != 512)        return false;

    // Only two FAT tables should exist
    if (!(bpb->ext_flags & EXT_FLAG_MIRROR) && (bpb->ext_flags & EXT_FLAG_ACT) > 1) return false;

    // FAT type is determined from the count of clusters
    uint32 sect_cnt = bpb->sect_cnt_32 - (bpb->cnt_rsv + bpb->cnt_fat * bpb->fatsz_32);
    uint32 clusters = sect_cnt / bpb->spc;
    
    if (clusters < 0x10000) return false;

    /*PartInfo __pi = { 0 };*/ PartInfo* pi = &_pi;

    pi->lba = lba;	pi->size = bpb->sect_cnt_32;
	pi->root_cluster = bpb->root_cluster;
	pi->clust_shift = __builtin_ctz(bpb->spc); // Calculate the shift for cluster size
	pi->total_sect = sect_cnt;
	pi->total_clust = clusters;
	pi->fat_sect[0] = lba + bpb->cnt_rsv;
	pi->fat_sect[1] = pi->fat_sect[0] + bpb->fatsz_32;
    if (bpb->ext_flags & EXT_FLAG_MIRROR) {
        pi->fat_sect[1] = pi->fat_sect[0];
	}
    if (bpb->ext_flags & EXT_FLAG_ACT) {
        pi->fat_sect[1] += bpb->fatsz_32;
	}
    if (bpb->volume_label[0] != 0) {
        wc_memcpy(pi->label, bpb->volume_label, 11);
        pi->label[11] = '\0'; // Ensure null-termination
    } else {
        wc_memset(pi->label, 0, sizeof(pi->label));
	}

	pi->current_cluster = bpb->root_cluster; // Set current cluster to root cluster
	pi->sect = 0; // Initialize current sector to 0
	pi->info_sect = bpb->info_sect ? (lba + bpb->info_sect) : 0; // Set FsInfo sector
	pi->data_sect = pi->fat_sect[0] + (bpb->cnt_fat * bpb->fatsz_32); // Data area starts after FATs
	//pi->last_used = clusters + CLUSTER_FIRST - 1; // Last used cluster is the last cluster

    return true;
}

DISC_INTERFACE* dldiGetInternal(void);

static void pwdinit();

int probe(/*DiskOps* ops, int partition, */void** param)
{
	uint32 lba = 0; // Start with the first sector
    uint8 g_buf[512];    MBR* mbr = (MBR*)g_buf;// Bpb510* bpb = (Bpb510*)g_buf;
    PartEntry	part[4];

    disc = dldiGetInternal();
	if (!disc->isInserted()) return FAT_ERR_NOFAT;

    //MBR mbr;
	disc->readSectors(0, 1, g_buf);
    if (mbr->sig != 0xAA55) return FAT_ERR_NOFAT;
    if (mbr->part[0].type != 0x00) {
        memcpy(part, mbr->part, sizeof(part) * 4);
        //Bpb510* bpb = NULL;
        for (int i = 0; i < 4; i++) {
            if (part[i].type == 0x00) continue; // Skip empty entries
            if (part[i].type == 0x0B || part[i].type == 0x0C) { // FAT16 or FAT32
                lba = part[i].lba;
                //break;
                disc->readSectors(lba, 1, g_buf);
                //if (check_fat(g_buf, lba)) {
                    //*lba = part[i].lba;
               //     return FAT_ERR_NONE;
               // }
                break;
            }
        }
       disc->readSectors(lba, 1, g_buf);
    }
    //else *lba = 0;
	bool safe = check_fat(g_buf, lba);
    return safe;// if (clusters > ) FAT_ERR_NONE : FAT_ERR_NOFAT;

}

int utf16to8(char* dst, const wchar_t* src, size_t dst_size)
{
    if (!dst || !src || dst_size == 0) {
        return 0; // Invalid parameters
    }
    char* p = dst;
    size_t remaining = dst_size - 1; // Leave space for null terminator
    while (*src && remaining > 0) {
        wchar_t wc = *src++;
        if (wc < 0x80) { // 1 byte UTF-8
            if (remaining < 1) break;
            *p++ = (char)wc;
            remaining--;
        } else if (wc < 0x800) { // 2 bytes UTF-8
            if (remaining < 2) break;
            *p++ = (char)((wc >> 6) | 0xC0);
            *p++ = (char)((wc & 0x3F) | 0x80);
            remaining -= 2;
        } else { // 3 bytes UTF-8
            if (remaining < 3) break;
            *p++ = (char)((wc >> 12) | 0xE0);
            *p++ = (char)(((wc >> 6) & 0x3F) | 0x80);
            *p++ = (char)((wc & 0x3F) | 0x80);
            remaining -= 3;
        }
    }
    *p = '\0'; // Null-terminate the string
	return p - dst; // Return the number of bytes written
}


wchar_t wchar2dbcs(wchar_t uni) {

}

#if 0
static wchar_t __toupper(wchar_t c) {
    if (c >= 'a' && c <= 'z') {
        return c - ('a' - 'A'); // Convert lowercase to uppercase
    }
    return c; // Return the character unchanged if it's not lowercase
}
#endif

int wstrtosfn(wchar_t* str, Sfn* sfn) {
	uint32 name_idx = 0; // Initialize name length
	uint8* name = (uint8*)sfn->name; 
	if (!sfn) return FAT_ERR_PARAM; // Invalid Sfn pointer

	while (*str && name_idx < 10) {
		if (*str == '.') {
            if (name_idx < 8) name_idx = 8;
            else return 0;// this is not fit for 8.3 
		}
		else {
            //name_idx += (*str < 0x100) ? 1 : 2;
            if (*str < 0x100) {
				name[name_idx++] = (uint8)wc_toupper(*str);
            }
            else {
                wchar2dbcs(*str);
            }
		}

	str++;
	}
	// Initialize the Sfn structure

	return 1;// this is fit for 8.3 

}

int strtosfn(char* str, Sfn* sfn) {
    if (!sfn) return FAT_ERR_PARAM; // Invalid Sfn pointer
    // Initialize the Sfn structure
}
//CString AltUTF8To16(LPCSTR lpMultiByteStr)
int utf8to16(const char* pdir, wchar_t* psplit, char separator)
{
	uint8* Z = (uint8*)pdir;

	while (*Z != separator) { //1 byte
		if (*Z < 0x80)  *psplit++ = (wchar_t)*Z++;
		else {
			uint32 val = 0, shift = 0;
			uint8 lead = (*Z++ ^ 0x80) << 1;
			for (; lead & 0x80; lead <<= 1, shift += 5)
				val = (val << 6) | (*Z++ ^ 0x80);
			val |= (uint32)lead << (shift - 1);
			*psplit++ = (wchar_t)val; // Convert UTF8 to wide char
		}
	}
	*psplit = '\0';
    if (*Z != '\0') Z++; // Skip the separator if it exists
	//*ppdir = (char*)++Z; // Update the pointer to the next character after the separator
	return Z- (uint8*)pdir;
    // ret;// Z - *ppdir; 
}

//fat cache -> 4k ; 8 sectors

typedef struct {
    uint32 count;  // count
    uint32 next_cluster; // Cluster number
} chain_info;

static uint32 chain_buf[128]; // Buffer to hold cluster chain information

uint32 get_next_cluster(uint32 cur_cluster)
{
	static uint32 cached_sector = 0; // Keep track of the last accessed sector

    //uint32 start_cluster = 0; // Initialize start cluster
    //uint32 cluster_buf[128 * 8];   // MBR* mbr = (MBR*)g_buf;// Bpb510* bpb = (Bpb510*)g_buf;
    // PartEntry	part[4];

    disc = dldiGetInternal();
    //if (!disc->isInserted()) return FAT_ERR_NOFAT;

    //MBR mbr;
    PartInfo* pi = &_pi;

    //uint32 sector = pi->fat_sect[0] + cur_cluster / 128;
    uint32 sector = pi->fat_sect[0] + cur_cluster / 128;
    uint32 delta = cur_cluster % 128;

    if (sector != cached_sector) {
        disc->readSectors(sector, 1, chain_buf);
        cached_sector = sector;
    }
    return chain_buf[delta]; // Return the next cluster
}



static uint8 crc4lfn(const uint8* name)
{
    uint32 sum = 0;
    for (int i = 0; i < 11; i++)
		sum = (sum >> 1) + (name[i]<<24) + (sum & (1<<24) ? 0x80000000 : 0);
    return (uint8)(sum>>24);
}

uint32 cluster2sector(uint32 cur_cluster) {
    PartInfo* pi = &_pi;
    return (pi->data_sect + ((cur_cluster - 2) << pi->clust_shift));
}

#define FILTER_DIR  0x10
#define FILTER_FILE 0x20

uint8 direntry_buffer[4096];// 4k
int32 dir_idx;
uint32 start_cluster;

static Sfn* get_sfn_entry(void) {
    // Get the current LFN entry from the directory buffer
    return (Sfn*)&direntry_buffer[(dir_idx & 0x7f) * sizeof(Sfn)];
}

static Sfn* proceed_idx(void) {
    //dir_idx++;
    //if (dir_idx >= 128) { // 128 entries per sector
    //    dir_idx = 0;
    //    disc->readSectors(cluster2sector(_pi.current_cluster) + dir_idx / (16 * 8), 8, direntry_buffer);
    //}
    dir_idx++;
    // Check if we have reached the end of the directory entries
    if (dir_idx << 25 == 0) {
        disc = dldiGetInternal();
        uint32 sector = cluster2sector(start_cluster); // Initialize sector to 0
        disc->readSectors(sector + dir_idx / (16 * 8), 8, direntry_buffer);
    }
	return get_sfn_entry(); // Return the next LFN entry
}



Sfn* assemble_lfn(wchar_t lfn[]) {
	const uint8 lfn_idx[13] = { 30, 28, 24, 22, 20, 18, 16, 14, 9, 7, 5, 3, 1 };
	Lfn* entry = (Lfn*)get_sfn_entry();
	// , * lfn_entry = (Lfn*)direntry_buffer;
	//wchar_t* p = lfn;

	//entry = 
	uint8   checksum = entry->checksum;
	uint32  count = entry->order ^ 0x40;
	if (count > 20) goto error; else lfn[count * 13] = '\0';
	for (wchar_t* p = &lfn[count * 13 - 1]; count > 0; entry = (Lfn*)proceed_idx(), count--) {
		for (int i = 0; i < 13; i++)
			*p-- = (wchar_t)(entry->raw[lfn_idx[i]]) | entry->raw[lfn_idx[i] + 1] << 8;
		if (checksum != entry->checksum || entry->attr != 0x0f || count != (entry->order & 0x1f)) goto error; // Checksum mismatch, invalid LFN entry
	}
	if (checksum != crc4lfn((const uint8*)entry)) goto error; // Checksum mismatch, invalid LFN entry
	return (Sfn*)entry; // Successfully assembled the long file name
error:
	lfn[0] = 0;
	return NULL; // Error in assembling the long file name
}

void sfn2lfn(Sfn* entry, wchar_t* lfn)
{
	// Convert a short file name entry to a long file name (LFN) representation
	// This function fills the 'lfn' buffer with the long file name from the Sfn entry
	//lfn[0] = 0; // Initialize the long file name buffer
	// Copy the short file name to the long file name buffer
	for (int i = 0; i < 11; *lfn++ = (wchar_t)entry->name[i], i++) {
        if (entry->name[i] == ' ') {
            if (i < 8) i = 8; // Skip spaces in the short file name 
            else break;
        }
        if (i == 8) {
            if (entry->name[8] == ' ') break;
            else    *lfn++ = '.'; // Add a dot before the extension if it exists
        }
	}
	*lfn = 0; // Null-terminate the string
}


int enum_dir_entry(Sfn** sfn, wchar_t lfn[], uint32 filter) 
{
    Sfn* entry;// , * lfn_entry = (Lfn*)direntry_buffer;
    //uint8 order;//
	//lfn[0] = 0; // Initialize the long file name buffer
    //Lfn* lfn = &lfn_entry[dir_idx & 0x7f];
    
    //for (entry = proceed_idx(), lfn[0] = 0; entry->name[0]; entry = proceed_idx(), lfn[0] = 0)
    //do
	while ((entry = proceed_idx())->name[0])
    {
        if(entry->name[0] == 0xE5) // Skip deleted entries
			continue; // Skip deleted entries

		// e5, 0f , unmatched entries
        if (entry->attr == 0x0f)
        { 
            entry = assemble_lfn(lfn);
            //entry = get_lfn_entry();
            //return dir_idx + 1;
            //continue;
        } else lfn[0] = 0;
        //if (entry->name[0] != 0xE5 && entry->attr != 0x0f) {
            if ((filter | entry->attr) == entry->attr) {
                // Check if the entry is a valid short file name entry
                if (lfn[0] == 0) sfn2lfn(entry, lfn);
                *sfn = entry;
                return dir_idx + 1;
            }
        //}
        // Convert the short file name to Sfn structure

        //next_entry //
    } //while (entry->name[0]);
    //while (lfn_entry[dir_idx].name[0] != 0x00 && lfn_entry[dir_idx].name[0] != 0xE5); // Continue until we find a valid entry or reach the end of the directory

    return 0;// dir_idx + 1;
}


int open_dir(uint32 cluster)// , & idx, filter, sfn);
{
    // This function should open a directory entry starting from 'start_cluster'.
    // The implementation details depend on the filesystem structure.
    // For now, we will just return FAT_ERR_NOFAT to indicate no FAT filesystem found.
    // In a real implementation, this function would read the directory entries and populate 'sfn'.
    dir_idx = -1;
#if 0
    disc = dldiGetInternal();
    uint32 sector = cluster2sector(cluster); // Initialize sector to 0
    disc->readSectors(sector + dir_idx / (16 * 8), 8, direntry_buffer);
#endif  
	start_cluster = cluster; // Set the starting cluster for directory entries
    return 1;// pass dir_idx_structure FAT_ERR_NONE;
}

typedef struct _pwd_struct {
    uint16 length;
    wchar_t name[255];
} pwd_struct;

static pwd_struct pwd;
//wchar_t pwd[256];

void pwdinit() {
    pwd.length = 0;
}

void pwdup()
{
    uint32 size = (uint32)pwd.length;
    while (pwd.name[--size] != '/') {}
    pwd.name[size] = '\0'; // Remove the last directory from the path       
    pwd.length = size; // Update the length of the path
}

void pwdcat(wchar_t* dir)
{
    if (*dir == '.')
    {
        if (*(dir + 1) == '.') pwdup();
        return;
    }
    uint32 size = (uint32)pwd.length;
    pwd.name[size++] = '/';
    //*ptr++ = '/';
    //for (*ptr++ = '/'; *dir; dir++)
    while (*dir) {
        pwd.name[size++] = *dir++;
    } 
    pwd.name[size] = '\0';
    pwd.length = size;
}



static uint32  enterdir(uint32 start_cluster, wchar_t* dirname)
{
	//chain_info cluster_chain[128]; // Array to hold the cluster chain
	//int count = build_cluster_chain(start_cluster, cluster_chain);
	wchar_t lfn[128]; // Buffer for long file name
    Sfn* sfn;
    if (*dirname == '\0') return 0;
    //    return start_cluster; // If the directory name is empty, return the current cluster

    open_dir(start_cluster);// , & idx, filter, sfn);
    while (enum_dir_entry(&sfn, lfn, FILTER_DIR)) {
        if (wc_wstrcasecmp(lfn, dirname) == 0) {
			uint32 sfn_cluster = (sfn->clust_hi << 16) | sfn->clust_lo; // Get the cluster number from the Sfn
            pwdcat(lfn);
            if (sfn_cluster == 0) return _pi.root_cluster;
            else return sfn_cluster;
        } 
	}
	return 0;
}

#if 0
void* stepdown(struct _reent* r, const char* path, uint32 is_dir) {

    if (!path || !*path) return NULL; // Invalid path
    //path UTF8 --> need unicode presentation
    //  
#define MAX_DEPTH 16
    wchar_t upath[MAX_DEPTH * 16], * pupath[MAX_DEPTH];
    //char* pstr = (char*)path;
    //PartInfo* pi = &_pi;
    uint32 start_cluster = _pi.current_cluster;

    char* p = strchr(path, ':');

    if (p) p++;
    else p = (char*)path; // No drive letter, start from the beginning
    while (*p == '\\' || *p == '/') {
        p++;
        start_cluster = _pi.root_cluster; // Start from root directory
    }
    int depth = 0;
    wchar_t* split = upath; // Pointer to the current position in the wide char path
    char* pdir = p; // Pointer to the current position in the wide char path

    //while (*p) 

    do {
        if (*p == '\\' || *p == '/' || *p == '\0') {
            // Handle directory separator
            // 
            pupath[depth++] = split;
            utf8to16(&pdir, &split, *p); // Convert UTF8 to wide char

        }

    } while (*++p);
    // pupath[depth++] = split;
    // utf8to16(&pdir, &split, '\0'); // Convert UTF8 to wide char
     // Now 'upath' contains the wide char representation of the path
    for (int i = 0; i < depth-is_dir; i++) {
        start_cluster = enterdir(start_cluster, pupath[i]);
		if (!start_cluster) return NULL; // Failed to enter directory
	}

    return NULL;

}
#else



static uint32 stepdown(const char* path, uint32 last_cluster, char separator)
{
    wchar_t upath[256];
    /*int length = */utf8to16(path, upath, separator); // Convert UTF8 to wide char
    return enterdir(last_cluster, upath);
}

static const char* slidedown(const char* path, uint32* last_cluster)
{
    if (!path || !*path) return (char*)path; // Invalid path
    uint32 cur_cluster = _pi.current_cluster;
    char* p = strchr(path, ':');
	//; // If there's a drive letter, skip it
	for (p = p ? p + 1 : (char*)path; *p == '\\' || *p == '/'; p++, cur_cluster = _pi.root_cluster) {} // Skip leading separators and set current cluster to root directory

    //path = p; // Update path to the current position after skipping separators
    //do {
	for (path = p; *p; p++)
		if (*p == '\\' || *p == '/') {// || *p == '\0') {
			uint32 ret_cluster = stepdown(path, cur_cluster, *p); // Step down the directory structure based on the current path segment
			if (!ret_cluster) break;
			cur_cluster = ret_cluster;
			path = p + 1;
		}
//} //while (*p++);
    *last_cluster = cur_cluster;
    return path;// pdir;// start_cluster;
}

#endif

DIR_ITER* _WC_diropen_r(struct _reent* r, DIR_ITER* dirState, const char* path)
{
    pwdinit();
    uint32 last_cluster;// = 0; // Variable to hold the last cluster
    const char* last_entry = slidedown(path, &last_cluster); // Step down the directory structure based on the given path
#if 1
    //if (!lastdir) {
    //    return (DIR_ITER * )FAT_ERR_NOFAT; // Failed to step down the directory structure
    //}

    if (*last_entry != '\0') { // If there are still characters left in the path
        uint32 ret_cluster = stepdown(last_entry, last_cluster, '\0'); // Step down to the last directory
        if (!ret_cluster) {
            r->_errno = ENOENT; // Set error code for "No such file or directory"
            return NULL; // Failed to step down to the last directory
        }
        else last_cluster = ret_cluster;
    }
#endif
	//if (last_cluster == 0) return NULL; // No valid cluster found
    //open_dir(last_cluster); // Open the directory at the specified cluster
    return (DIR_ITER * )last_cluster;// (DIR_ITER*)1;
}

static uint32 build_cluster_chain(uint32 start_cluster, chain_info* chain_start)
{
    chain_start->count = 0;
    chain_start->next_cluster = 0;

    // This function builds a cluster chain starting from 'start_cluster' and fills 'chain' with the wide char representation of the cluster chain.
    // The implementation details depend on the filesystem structure.
    // For now, we will just return FAT_ERR_NOFAT to indicate no FAT filesystem found.
    if (start_cluster < 2 || start_cluster > 0xFFFFFFF6) {
        return FAT_ERR_PARAM; // Invalid start cluster
    }
    if (!chain_start) {
        return FAT_ERR_PARAM; // Invalid chain pointer
    }
    uint32 next_cluster, cur_cluster = start_cluster; // Initialize current cluster
    PartInfo* pi = &_pi;
    //uint32 sector = start_cluster / 128;
    //uint32 delta = start_cluster % 128;
    chain_info* chain = chain_start; // Initialize the chain count

    do {
        //prev_start_cluster = start_cluster; // Store the previous cluster value
        next_cluster = get_next_cluster(cur_cluster);
        if (next_cluster > 0x0FFFFFF6) {
            return chain - chain_start; // end
        }
        else if (next_cluster - cur_cluster == 1) chain->count++;
        else if (next_cluster < pi->total_clust) {
            //chain++;
            chain++->next_cluster = next_cluster;
            chain->count = 0; chain->next_cluster = 0;
        }
        else {
            return FAT_ERR_BROKEN; // Invalid cluster value
        }
		cur_cluster = next_cluster; // Update current cluster to the next cluster
    } while (next_cluster < 0x0ffffff7);
}


struct _FILE_STRUCT {
    uint32_t             ptr;
    uint32_t             mode;
    uint32_t             filesize;
    uint32_t			 chainCount;
    uint32_t             startCluster;
    chain_info           cluster_chain[(512 / 4 - 5)/2];
};

typedef struct _FILE_STRUCT FILE_STRUCT;


static uint32  open_file(uint32 dir_cluster, wchar_t *ufile, void* fileStruct)
{
    Sfn* sfn;
    wchar_t lfn[128]; // Buffer for long file name
    
    open_dir(dir_cluster);// , & idx, filter, sfn);
    while (enum_dir_entry(&sfn, lfn, FILTER_FILE)) {
        if (wc_wstrcasecmp(lfn, ufile) == 0) {
			FILE_STRUCT* fs = (FILE_STRUCT*)fileStruct;
			fs->ptr = 0; // Initialize file pointer
            fs->mode = 0; // Initialize file mode
			fs->filesize = sfn->size; // Get the file size from the Sfn
			fs->startCluster = (sfn->clust_hi << 16) | sfn->clust_lo; // Get the start cluster from the Sfn

            fs->chainCount = build_cluster_chain(fs->startCluster, (chain_info*) & fs->cluster_chain);

            //uint32 sfn_cluster = (sfn->clust_hi << 16) | sfn->clust_lo; // Get the cluster number from the Sfn
            return fs->startCluster;
        }
    }
    return 0;
}

int _WC_open_r(struct _reent* r, void* fileStruct, const char* path, int flags, int mode)
{
	uint32 last_cluster = 0; // Variable to hold the last cluster
	const char* last_entry = slidedown(path, &last_cluster); // Step down the directory structure based on the given path
#if 0
    if (!filename) {
        r->_errno = ENOENT;
		//return FAT_ERR_NOFAT; // Failed to step down the directory structure
	}
#endif
    wchar_t upath[128];
    //char* pdir = (char*)filename; // Pointer to the current position in the wide char path
    utf8to16(last_entry, upath, '\0'); // Convert UTF8 to wide char


    if (!open_file(last_cluster, upath, fileStruct)) {
        r->_errno = ENOENT;
        return -1; // File not found
    }

	// Here, we would typically open the file and return a file descriptor.

    return (int)fileStruct;

}

int _WC_close_r(struct _reent* r, void* fd)
{
}

ssize_t _WC_write_r(struct _reent* r, void* fd, const char* ptr, size_t len)
{
        // This function should write 'len' bytes from 'ptr' to the file represented by 'fd'.
    // The implementation details depend on the filesystem structure.
    // For now, we will just return FAT_ERR_NOFAT to indicate no FAT filesystem found.
    return FAT_ERR_NOFAT;
}

uint32 iterate_file(FILE_STRUCT* fs, uint32* remain)
{
    uint32 target_cluster_order = fs->ptr >> (_pi.clust_shift + 9);
    uint32* cluster_ptr = (uint32*) & fs->startCluster;
    uint32 cur_clusted_cluster, clusted_count;
    
    // = *cluster_ptr++;
    //uint32 sum_cluster_count = clusted_count;
   // while (target_cluster_order > sum_cluster_count) 
    
    for (cur_clusted_cluster = *cluster_ptr++, clusted_count = *cluster_ptr++; 
        target_cluster_order >= clusted_count;
        cur_clusted_cluster = *cluster_ptr++, clusted_count = *cluster_ptr++)
        //cur_clusted_cluster = *cluster_ptr++;
        //sum_cluster_count += *cluster_ptr++;
        //sum_cluster_count ++;
        target_cluster_order = target_cluster_order -1 - clusted_count;
    {}
    if (remain) *remain = clusted_count - target_cluster_order;
    return cur_clusted_cluster + target_cluster_order;
}

uint32 cachesec;
char cache[4096]; // Cache buffer for file data

char* cache_set(FILE_STRUCT* fs)
{
	// This function sets the cache for the given file pointer.



	uint32 cur_sector = cluster2sector(iterate_file(fs, NULL)) + 8 * (( fs->ptr >> 12)&0x07);
    // Calculate the current sector based on the file pointer
    cachesec = cur_sector;
    
    // Calculate the sector number based on the file pointer
    //if (fs->ptr >> 12 != cur_sector >> 12) { // Check if we need to load a new sector
        disc = dldiGetInternal();
        disc->readSectors(cur_sector, 8, cache); // Load the current sector into the buffer
    //}
    return cache; // No cache hit
}

char* cache_get(FILE_STRUCT* fs)
{
    uint32 cur_sector = cluster2sector(iterate_file(fs, NULL)) + 8 * ((fs->ptr >> 12) & 0x07);
	// This function checks if the cache has the data for the given file pointer.
    if (cur_sector != cachesec)
    {
        disc = dldiGetInternal();
        disc->readSectors(cur_sector, 8, cache); // Load the current sector into the buffer
        cachesec = cur_sector;
    }
    // The implementation details depend on the caching mechanism.
    // For now, we will just return false to indicate no cache hit.
    return cache; // No cache hit
}

ssize_t _WC_read_r(struct _reent* r, void* fd, char* ptr, size_t len)
{
    FILE_STRUCT* fs = (FILE_STRUCT*)fd;

    len = min(fs->filesize - fs->ptr, len);
    //    (len > fs->filesize - fs->ptr) ? (fs->filesize - fs->ptr) : len; // Adjust length to not exceed file size

    // load prolog
    // ptr -> cluster -> sector -> idx
    uint32 idx = fs->ptr;

    uint32 target_cluster_order = fs->ptr >> (_pi.clust_shift + 9);
    uint32 cluster_delta = fs->ptr ^ (target_cluster_order << (_pi.clust_shift + 9));
    uint32 cluster_shift = cluster_delta >> 9;
    //cluster_delta = 
    //uint32 sector_delta = cluster_delta ^ (cluster_shift << 9);



        // check cluster boundary for ieration
        // 
        // load sector boundary
    uint32 cidx;// , cur_sector = cluster2sector(iterate_file(fs));
	char* cptr; // Pointer to the cache data
#if 0
	if (fs->ptr << 20 != 0) {
		// load prolog
        if (cache_hit(fs->ptr)) {
            memcpy(ptr, cache_get(fs->ptr), len);
            fs->ptr += len; // Update the file pointer
            return len; // Return the number of bytes read
		}
	}
#else
    if ((cidx = (fs->ptr & 0xFFF)) && (cptr = cache_get(fs))) {
        uint32 remain = 4096 - cidx;
        uint32 sz = min(len, remain);// len < remain ? len : remain; // Adjust size to not exceed the remaining length
        // Check if we are not at the start of a sector
  //      if (remain + len > 4096) {
  //          len = 4096 - remain; // Adjust length to not exceed the sector size
		//}
		memcpy(ptr, cptr + cidx, sz);
		fs->ptr += sz; // Update the file pointer
		len -= sz; // Decrease the remaining length to read
        ptr += sz;
		if (len == 0) return sz; // Return the number of bytes read
	}

#endif

#if 0
    while (fs->ptr < fs->filesize && len > 0)
    {
        if (fs->ptr >> 12 != cur_sector >> 12) { // Check if we need to load a new sector
            disc = dldiGetInternal();
            disc->readSectors(cur_sector, 8, direntry_buffer); // Load the current sector into the buffer
        }
        uint32 offset = fs->ptr & 0xFFF; // Calculate the offset within the sector
        uint32 bytes_to_read = (len < (4096 - offset)) ? len : (4096 - offset); // Determine how many bytes to read
        memcpy(ptr, &direntry_buffer[offset], bytes_to_read); // Copy data from the buffer to the output pointer
        ptr += bytes_to_read; // Move the output pointer forward
        fs->ptr += bytes_to_read; // Update the file pointer
        len -= bytes_to_read; // Decrease the remaining length to read
        if (fs->ptr >= fs->filesize) break; // Stop if we have reached the end of the file
	}
#else

    // load 4k boundary
    //for (uint32 count = len >> 12; count > 0; count--)
    uint32 count = len >> 12;
    while(count>0)
	{
		//if (fs->ptr >= fs->filesize) break; // Stop if we have reached the end of the file
        //aggrigate clustered sector
        uint32 num_blocks = 1;
        uint32 cluster_count;
        uint32 max_cont;
        uint32 cur_sector = cluster2sector(iterate_file(fs, &cluster_count)) + 8 * ((fs->ptr >> 12) & 0x07);
        //if (cluster_count >= 0)

        max_cont = cluster_count * 8 + (8 - ((fs->ptr >> 12) & 0x07));

            num_blocks = min(max_cont, count);
        //if (fs->ptr >> 12 != cur_sector >> 12) { // Check if we need to load a new sector
            disc = dldiGetInternal();
            uint32 sz = 0x1000 * num_blocks;
            disc->readSectors(cur_sector, 8 * num_blocks, ptr); // Load the current sector into the buffer
            
            
            fs->ptr += sz; ptr += sz;
            len -= sz;
            count -= num_blocks;
    }
#endif
    // load epilog
    if (len > 0) {
		char* cptr = cache_set(fs); // Cache the data read from the file
        // read 4kb on cache
		memcpy(ptr, cptr, len); // Copy remaining data from the buffer to the output pointer 
        // cache to destination 
        fs->ptr += len;
    }
    return fs->ptr - idx; // Return the number of bytes read

   // return FAT_ERR_NOFAT;
}

typedef long off32_t;
typedef off32_t off_t;

//ssize_t _WC_read_r(struct _reent* r, void* fd, char* ptr, size_t len)
off_t _WC_seek_r(struct _reent* r, void* fd, off_t pos, int dir)
{
    FILE_STRUCT* fs = (FILE_STRUCT*)fd;
    //fs->ptr =

    long ptr = (dir == 0) ? pos : ((dir == 2) ? (fs->filesize - pos) : fs->filesize + pos);
    if (ptr >= 0 && ptr <= fs->filesize) fs->ptr = ptr;
//    if (dir == 0) fs->ptr = pos;
//    else if (dir == 0) fs->ptr = pos; 
}

off_t _WC_ftell_r(struct _reent* r, void* fd)
{
    FILE_STRUCT* fs = (FILE_STRUCT*)fd;
    return fs->ptr;
}