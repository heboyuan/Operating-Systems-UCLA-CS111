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

//====================================================//
//Utility                                             //
//====================================================//
void format_time(uint32_t time_stamp, char* buf) {
	time_t temp = time_stamp;
	struct tm ts = *gmtime(&temp);
	strftime(buf, 80, "%m/%d/%y %H:%M:%S", &ts);
}

void indirect(int fd, int block_size, int upper_inode, int level, int cur_block, int block_offset){
    int buffer[block_size/4];
    memset(buffer, 0, sizeof(buffer));
    pread(fd, buffer, block_size, cur_block * block_size);
    int i;
    for(i = 0; i < block_size/4 ;i++){
        if(buffer[i] != 0){
            fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n", upper_inode, level, block_offset, cur_block, buffer[i]);
            if(level != 1){
                indirect(fd, block_size, upper_inode, level-1, buffer[i], block_offset);
            }else{
              block_offset++;
            }
        }
        else{
            if(level == 1)
                block_offset++;
            else if (level == 2){
                block_offset += 256;
            }
            else{
                block_offset += 65536;
            }
        }
    }
}

//====================================================//
//Main                                                //
//====================================================//
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

  struct ext2_inode inode;

  void* temp_inode = (void*)&inode;
  int inode_offset = block_size * cur_group.bg_inode_table;
  for(unsigned int index = 0; index < superblock.s_inodes_count; index++){
    //====================================================//
    //part5 inode summary                                 //
    //====================================================//
    pread(fd, temp_inode, sizeof(inode), inode_offset+(index*sizeof(inode)));
    if(inode.i_mode != 0 && inode.i_links_count != 0){
      fprintf(stdout, "INODE,%d,",(index+1));
      char file_format = '?';
      if(S_ISREG(inode.i_mode)){
        file_format = 'f';
      }
      else if(S_ISDIR(inode.i_mode)){
        file_format = 'd';
      }
      else if(S_ISLNK(inode.i_mode)){
        file_format = 's';
      }

      fprintf(stdout, "%c,%o,%d,%d,%d,",
        file_format,
        inode.i_mode & 0xFFF,
        inode.i_uid,
        inode.i_gid,
        inode.i_links_count);

      char i_change_time[80];
      char i_modify_time[80];
      char i_access_time[80];

      format_time(inode.i_ctime, i_change_time);
			format_time(inode.i_mtime, i_modify_time);
			format_time(inode.i_atime, i_access_time);

      fprintf(stdout, "%s,%s,%s,%d,%d",
        i_change_time,
        i_modify_time,
        i_access_time,
        inode.i_size,
        inode.i_blocks);

      int num_blocks = EXT2_N_BLOCKS;
      for (int i = 0; i < num_blocks; i++) {
				fprintf(stdout, ",%d", inode.i_block[i]);
			}
			fprintf(stdout, "\n");

      //====================================================//
      //part6 directory entries                             //
      //====================================================//

      //------------
      int cur_block;
      //------------
      if(file_format == 'd'){
        for(cur_block = 0; cur_block < num_blocks; cur_block++){
          if(inode.i_block[cur_block] != 0){
            struct ext2_dir_entry entry;
            unsigned int cur_entry;
            for(cur_entry = 0; cur_entry < block_size; cur_entry += entry.rec_len){
              pread(fd, &entry, sizeof(entry), inode.i_block[cur_block]*block_size + cur_entry);
              if(entry.inode != 0){
                fprintf(stdout,"DIRENT,%d,%d,%d,%d,%d,'%s'\n",(index+1),cur_entry,
                entry.inode, entry.rec_len, entry.name_len, entry.name);
              }
              //fprintf(stdout,"cur_entry: %i\n", cur_entry);
              //fprintf(stdout,"cur_block: %i\n num_blocks: %i\n\n", cur_block, num_blocks);
            }
          }else{
            //fprintf(stdout,"cur_block: %i\n about to break\n\n", cur_block);
            break;
          }
        }
      }

      //====================================================//
      //part7 indirect block references                     //
      //====================================================//
      if(file_format == 'd' || file_format == 'f'){
        indirect(fd, block_size, index + 1, 1, inode.i_block[12], 12);
        indirect(fd, block_size, index + 1, 2, inode.i_block[13], 268);
        indirect(fd, block_size, index + 1, 3, inode.i_block[14], 65804);
      }
    }

  //====================================================
  // for multiple group
  //--------------------------------------------------
  // }
  //===================================================
  }
}
