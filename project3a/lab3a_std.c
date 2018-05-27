#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <assert.h>
#include <math.h>

#include "ext2_fs.h"

/* Exit Codes */
// EXIT_SUCCESS     0
#define EXIT_ARG    1
#define EXIT_ERR    2

#define BASE_OFFSET	1024			// beginning of super block
#define BLOCK_OFFSET(block) (BASE_OFFSET + (block-1) * block_size)

/* Defined i_mode values */
#define EXT2_S_IRWXU	0x01C0	    // user access rights mask
#define EXT2_S_IRWXG	0x0038	    // group access rights mask
#define EXT2_S_IRWXO	0x0007	    // others access rights mask

/* Globals */
struct ext2_super_block sb;			// super block struct
struct ext2_group_desc* gds;		// group descriptors

static int fsfd = -1;      			// file system file descriptor
static unsigned int block_size = 0; // block size
static unsigned int n_groups = 0;	// number of groups
int* inode_bitmap;

/* Utility Functions */
void format_time(uint32_t timestamp, char* buf) {
	time_t epoch = timestamp;
	struct tm ts = *gmtime(&epoch);
	strftime(buf, 80, "%m/%d/%y %H:%M:%S", &ts);
}

void free_all() {
	if (gds != NULL) {
		free(gds);
	}
	if(inode_bitmap != NULL){
		free(inode_bitmap);
	}
	close(fsfd);
}

/* Error Reporting */
void print_error(const char *msg, int exit_code) {
	if (errno) {
		fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    } else {
		fprintf(stderr, "%s\n", msg);
    }

	free_all();
	exit(exit_code);
}

/* Analysis Functions */

// superblock summary
void scan_sb() {

	// Superblock located at byte offset 1024 from beginning
	if (pread(fsfd, &sb, sizeof(struct ext2_super_block), 1024) < 0) {
		print_error("Error reading superblock", EXIT_ERR);
	}

	if (sb.s_magic != EXT2_SUPER_MAGIC) {
		print_error("Error: not an ext2 file system", EXIT_ERR);
	}

	block_size = EXT2_MIN_BLOCK_SIZE << sb.s_log_block_size,

	printf("SUPERBLOCK,%u,%u,%u,%u,%u,%u,%u\n",
			sb.s_blocks_count,
			sb.s_inodes_count,
			block_size,
			sb.s_inode_size,
			sb.s_blocks_per_group,
			sb.s_inodes_per_group,
			sb.s_first_ino);
}

// group summary
void scan_groups() {
	n_groups = 1 + (sb.s_blocks_count-1) / sb.s_blocks_per_group;
	unsigned int descr_list_size = n_groups * sizeof(struct ext2_group_desc);

	// Allocate memory for group descriptors
	gds = malloc(descr_list_size);
	if(gds == NULL){
		print_error("malloc failed", EXIT_ERR);
	}

	if (pread(fsfd, gds, descr_list_size, BASE_OFFSET + block_size) < 0) {
		print_error("Error reading group descriptor", EXIT_ERR);
	}

	// Keep track of remainders for groups blocks/inodes
	uint32_t block_remainder = sb.s_blocks_count;
	uint32_t blocks_in_group = sb.s_blocks_per_group;

	uint32_t inodes_remainder = sb.s_inodes_count;
	uint32_t inodes_in_group = sb.s_inodes_per_group;

	for (int i = 0; i < n_groups; i++) {
		if (block_remainder < sb.s_blocks_per_group) {
			blocks_in_group = block_remainder;
		}

		if (inodes_remainder < sb.s_inodes_per_group) {
			inodes_in_group = inodes_remainder;
		}

		printf("GROUP,%u,%u,%u,%u,%u,%u,%u,%u\n",
				i,
				blocks_in_group,
				inodes_in_group,
				gds[i].bg_free_blocks_count,
				gds[i].bg_free_inodes_count,
				gds[i].bg_block_bitmap,
				gds[i].bg_inode_bitmap,
				gds[i].bg_inode_table);

		block_remainder -= sb.s_blocks_per_group;
		inodes_remainder -= sb.s_inodes_per_group;
	}
}

// free block entries
void scan_block_bitmap() {
	unsigned long i;
	//go through each group
	for(i = 0; i < n_groups; i++){
		unsigned long j;
		for(j = 0; j < block_size; j++){
			//read one byte at a time from the block's bitmap
			uint8_t byte;
			if (pread(fsfd, &byte, 1, (block_size * gds[i].bg_block_bitmap) + j) < 0) {
				print_error("Error with pread", EXIT_ERR);
			}
			int mask = 1;
			unsigned long k;
			//look at the byte bit by bit, zero indicating a free block.
			for(k = 0; k < 8; k++){
				if((byte & mask) == 0){
					fprintf(stdout, "BFREE,%lu\n", i * sb.s_blocks_per_group + j * 8 + k + 1);
				}
				mask <<=1;
			}
		}
	}
}

// free inode entries
void scan_inode_bitmap() {
	unsigned long i;
	//go through each group
	inode_bitmap = malloc(sizeof(uint8_t) * n_groups * block_size);
	if(inode_bitmap == NULL){
		print_error("malloc failed", EXIT_ERR);
	}
	for(i = 0; i < n_groups; i++){
		unsigned long j;
		for(j = 0; j < block_size; j++){
			//read one byte at a time from the block's bitmap
			uint8_t byte;
			if (pread(fsfd, &byte, 1, (block_size * gds[i].bg_inode_bitmap) + j) < 0) {
				print_error("Error with pread", EXIT_ERR);
			}
			inode_bitmap[i + j] = byte;
			int mask = 1;
			unsigned long k;
			//look at the byte bit by bit, zero indicating a free block.
			for(k = 0; k < 8; k++){
				if((byte & mask) == 0){
					fprintf(stdout, "IFREE,%lu\n", i * sb.s_inodes_per_group + j * 8 + k + 1);
				}
				mask <<=1;
			}
		}
	}
}

// directory indirect block references
void scan_dir_indirects(struct ext2_inode* inode, int inode_no, uint32_t block_no, unsigned int size, int level) {
	uint32_t n_entries = block_size/sizeof(uint32_t);
	uint32_t entries[n_entries];
	memset(entries, 0, sizeof(entries));
	if (pread(fsfd, entries, block_size, BLOCK_OFFSET(block_no)) < 0) {
		print_error("Error with pread", EXIT_ERR);
	}

	unsigned char block[block_size];
	struct ext2_dir_entry *entry;

	for (int i = 0; i < n_entries; i++) {
		if (entries[i] != 0) {
			if (level == 2 || level == 3) {
				scan_dir_indirects(inode, inode_no, entries[i], size, level-1);
			}

			if (pread(fsfd, block, block_size, BLOCK_OFFSET(entries[i])) < 0) {
				print_error("Error with pread", EXIT_ERR);
			}
			entry = (struct ext2_dir_entry *) block;

			while((size < inode->i_size) && entry->file_type) {
				char file_name[EXT2_NAME_LEN+1];
				memcpy(file_name, entry->name, entry->name_len);
				file_name[entry->name_len] = 0;
				if (entry->inode != 0) {
					printf("DIRENT,%d,%d,%d,%d,%d,'%s'\n",
							inode_no,
							size,
							entry->inode,
							entry->rec_len,
							entry->name_len,
							file_name);
				}
				size += entry->rec_len;
				entry = (void*) entry + entry->rec_len;
			}
		}
	}
}

// scan directory
void scan_directory(struct ext2_inode* inode, int inode_no) {
	unsigned char block[block_size];
	struct ext2_dir_entry *entry;
	unsigned int size = 0;
	// direct blocks
	for (int i = 0; i < EXT2_NDIR_BLOCKS; i++) {
		if (pread(fsfd, block, block_size, BLOCK_OFFSET(inode->i_block[i])) < 0) {
			print_error("Error with pread", EXIT_ERR);
		}
		entry = (struct ext2_dir_entry *) block;

		while((size < inode->i_size) && entry->file_type) {
			char file_name[EXT2_NAME_LEN+1];
			memcpy(file_name, entry->name, entry->name_len);
			file_name[entry->name_len] = 0;
			if (entry->inode != 0) {
				printf("DIRENT,%d,%d,%d,%d,%d,'%s'\n",
						inode_no,
						size,
						entry->inode,
						entry->rec_len,
						entry->name_len,
						file_name);
			}
			size += entry->rec_len;
			entry = (void*) entry + entry->rec_len;
		}
	}

	// indirect block scanning
	if (inode->i_block[EXT2_IND_BLOCK] != 0) {
		scan_dir_indirects(inode, inode_no, inode->i_block[EXT2_IND_BLOCK], size, 1);
	}
	if (inode->i_block[EXT2_DIND_BLOCK] != 0) {
		scan_dir_indirects(inode, inode_no, inode->i_block[EXT2_DIND_BLOCK], size, 2);
	}
	if (inode->i_block[EXT2_TIND_BLOCK] != 0) {
		scan_dir_indirects(inode, inode_no, inode->i_block[EXT2_TIND_BLOCK], size, 3);
	}
}

// indirect block references
void scan_indirects(int owner_inode_no, uint32_t block_no, int level, uint32_t curr_offset) {
	uint32_t n_entries = block_size/sizeof(uint32_t);
	uint32_t entries[n_entries];
	memset(entries, 0, sizeof(entries));
	if (pread(fsfd, entries, block_size, BLOCK_OFFSET(block_no)) < 0) {
		print_error("Error with pread", EXIT_ERR);
	}
	assert(level == 1 || level == 2 || level == 3);
	for (int i = 0; i < n_entries; i++) {
		if (entries[i] != 0) {
			printf("INDIRECT,%u,%u,%u,%u,%u\n",
					owner_inode_no,
					level,
					curr_offset,
					block_no,
					entries[i]);
			if (level == 2 || level == 3) {
				scan_indirects(owner_inode_no, entries[i], level-1, curr_offset);
			}
		}else{
      if (level == 1)
				++curr_offset;
			else if (level == 2 || level == 3) {
				curr_offset += level == 2 ? 256 : 65536;
			}
    }
	}
}

// inode summary
void scan_inodes() {
	int inode_no;
	struct ext2_inode inode;

	for (int i = 0; i < n_groups; i++) {
		// First read root directory (inode_no = 2)
		// Afterwards, set to first non-reserved inode
		for (inode_no = 2; inode_no < sb.s_inodes_count; inode_no++) {

			int valid_inode = 1;
			int inode_found = 0;

			// Check if inode is valid
			for (int j = 0 ; j < block_size; j++) {
				uint8_t byte = inode_bitmap[i + j];
				int mask = 1;
				for (int k = 0; k < 8; k++) {
					unsigned long curr_inode_no = i * sb.s_inodes_per_group + j * 8 + k + 1;
					if (curr_inode_no == inode_no) {
						inode_found = 1;
						if ((byte & mask) == 0) {
							valid_inode = 0;
						}
						break;
					}
					mask <<= 1;
					if (inode_found) {
						break;
					}
				}
			}

			if (!inode_found) {
				print_error("Error: corrupted bitmap", EXIT_ERR);
			}

			if (!valid_inode) {
				continue;
			}

			off_t offset = BLOCK_OFFSET(gds[i].bg_inode_table) + (inode_no-1) * sizeof(struct ext2_inode);

			if (pread(fsfd, &inode, sizeof(struct ext2_inode), offset) < 0) {
				print_error("Error with pread", EXIT_ERR);
			}

			// Get inode file format
			char file_type = '?';
			if (S_ISDIR(inode.i_mode)) {
				file_type = 'd';
			} else if (S_ISREG(inode.i_mode)) {
				file_type = 'f';
			} else if (S_ISLNK(inode.i_mode)) {
				file_type = 's';
			}

			// Get mode (permissions)
			uint16_t mode = inode.i_mode & (EXT2_S_IRWXU | EXT2_S_IRWXG | EXT2_S_IRWXO);

			// Creation time
			char i_ctime[80];
			char i_mtime[80];
			char i_atime[80];

			format_time(inode.i_ctime, i_ctime);
			format_time(inode.i_mtime, i_mtime);
			format_time(inode.i_atime, i_atime);

			printf("INODE,%d,%c,%o,%u,%u,%u,%s,%s,%s,%u,%u",
					inode_no,
					file_type,
					mode,
					inode.i_uid,
					inode.i_gid,
					inode.i_links_count,
					i_ctime,
					i_mtime,
					i_atime,
					inode.i_size,
					inode.i_blocks);

			// Print block addresses
			for (int k = 0; k < EXT2_N_BLOCKS; k++) {
				printf(",%u", inode.i_block[k]);
			}
			printf("\n");

			if(file_type == 'd'){
				assert(S_ISDIR(inode.i_mode));
				scan_directory(&inode, inode_no);
			}
			// Check indirect blocks
			if (inode.i_block[EXT2_IND_BLOCK] != 0) {
				scan_indirects(inode_no, inode.i_block[EXT2_IND_BLOCK], 1, 12);
			}
			if (inode.i_block[EXT2_DIND_BLOCK] != 0) {
				scan_indirects(inode_no, inode.i_block[EXT2_DIND_BLOCK], 2, 268);
			}
			if (inode.i_block[EXT2_TIND_BLOCK] != 0) {
				scan_indirects(inode_no, inode.i_block[EXT2_TIND_BLOCK], 3, 65804);
			}

			// If currently reading root directory, go to first
			// non-reserved inode for next loop
			if (inode_no == 2) {
				inode_no = sb.s_first_ino - 1;
			}
		}
	}
}

int main(int argc, char **argv) {
    if (argc != 2) {
        print_error("Error: invalid number of arguments\nUsage: lab3a IMG_FILE", EXIT_ARG);
    }

    // Open file system image for read
    const char *fs_img = argv[1];
    fsfd = open(fs_img, O_RDONLY);
    if (fsfd < 0) {
        print_error("Error opening file system", EXIT_ERR);
    }

	scan_sb();
	scan_groups();
	scan_block_bitmap();
	scan_inode_bitmap();
	scan_inodes();

	free_all();
    exit(EXIT_SUCCESS);
}
