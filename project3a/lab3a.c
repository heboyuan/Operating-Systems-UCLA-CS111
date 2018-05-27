#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include "ext2_fs.h"

int main(int argc, char **argv){
  if(argc < 2){
    fprintf(stderr, "Error: Invalid arguement\n");
    exit(1);
  }
  int fd = open(argv[1], O_RDONLY);
  if(fd < 0){
    fprintf(stderr, "Error: Cannot open file\n");
    exit(1);
  }

  //====================================================//
  //part1 superblock                                    //
  //====================================================//
  struct ext2_super_block superblock;
  pread(fd, &superblock, sizeof(superblock), 1024);

  unsigned int block_size = 1024 << superblock.s_log_block_size;
  fprintf(stdout, "SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n", superblock.s_blocks_count,
  superblock.s_inodes_count, block_size, superblock.s_inode_size, superblock.s_blocks_per_group,
  superblock.s_inodes_per_group, superblock.s_first_ino);

  //====================================================
  // for multiple group
  //--------------------------------------------------
  // int num_group = superblock.s_blocks_count / superblock.s_blocks_per_group;
  // int group_id;
  // for(group_id = 0; group_id < num_group; group_id++){
  //====================================================

  //====================================================//
  //part2 group                                         //
  //====================================================//
  struct ext2_group_desc cur_group;

  //====================================================
  // for multiple group
  //--------------------------------------------------
  // pread(fd, &cur_group, sizeof(cur_group), 2048 + group_id*32);
  //====================================================
  pread(fd, &cur_group, sizeof(cur_group), 2048 );
  //====================================================
  // for multiple group
  //--------------------------------------------------
  //
  // int blocks_in_group = superblock.s_blocks_per_group;
  // int inodes_in_group = superblock.s_inodes_per_group;
  // if(group_id = num_group - 1){
  //   blocks_in_group = superblock.s_blocks_count%superblock.s_blocks_per_group;
  //   inodes_in_group = superblock.s_inodes_count%superblock.s_inodes_per_group;
  // }
  //
  //
  // fprintf(stdout, "GROUP,%d,%d,%d,%d,%d,%d,%d,%d\n", group_id, blocks_in_group,
  // inodes_in_group, cur_group.bg_free_blocks_count, cur_group.bg_free_inodes_count,
  // cur_group.bg_block_bitmap, cur_group.bg_inode_bitmap, cur_group.bg_inode_table)
  //====================================================

  int blocks_in_group = superblock.s_blocks_count%superblock.s_blocks_per_group;
  int inodes_in_group = superblock.s_inodes_count%superblock.s_inodes_per_group;
  if(blocks_in_group == 0){
    blocks_in_group = superblock.s_blocks_per_group;
  }
  if(inodes_in_group == 0){
    inodes_in_group = superblock.s_inodes_per_group;
  }

  fprintf(stdout, "GROUP,%d,%d,%d,%d,%d,%d,%d,%d\n", 0, blocks_in_group,
  inodes_in_group, cur_group.bg_free_blocks_count, cur_group.bg_free_inodes_count,
  cur_group.bg_block_bitmap, cur_group.bg_inode_bitmap, cur_group.bg_inode_table);


  //====================================================//
  //part3 free block                                    //
  //====================================================//
  unsigned int byte;
  unsigned char buffer;
  for(byte = 0; byte < block_size; byte++){
    pread(fd, &buffer, 1, cur_group.bg_block_bitmap*block_size + byte);
    int bit;
    for(bit = 0; bit < 8; bit++){
      if((buffer & (1 << bit)) == 0){
        fprintf(stdout, "BFREE,%d\n", (byte*8)+(bit+1));
      }
    }
  }

  //====================================================//
  //part4 free inode                                    //
  //====================================================//
  for(byte = 0; byte < block_size; byte ++){
    int buffer;
    pread(fd, &buffer, 1, cur_group.bg_inode_bitmap*block_size + byte);
    int bit;
    for(bit = 0; bit < 8; bit++){
      if((buffer & (1 << bit)) == 0){
        fprintf(stdout, "IFREE,%d\n", byte*8+bit+1);
      }
    }
  }

  //====================================================//
  //part5 inode summary                                 //
  //====================================================//


  //====================================================
  // for multiple group
  //--------------------------------------------------
  // }
  //===================================================
}
