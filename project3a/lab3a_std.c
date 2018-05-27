#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include<time.h>
#include<sys/time.h>
#include "ext2_fs.h"

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
		fprintf(stderr, "Error! Please specify a file to analyze and try again.\n");
		exit(1);
	}
    int FSimagefd = open(argv[1], O_RDONLY);
	if(FSimagefd < 0)
	{
		fprintf(stderr, "Error: Unable to open file! Exiting now....\n");
		exit(2);
	}

	//create output file
	/*
	int stringsize = strlen(argv[1]);
	char temp_name[stringsize];
	int name_set_index = 0;
	while (name_set_index < stringsize-4)
	{
	    temp_name[name_set_index] = argv[1][name_set_index];
	    name_set_index++;
	}
	temp_name[name_set_index] = '.';
	temp_name[name_set_index+1] = 'c';
	temp_name[name_set_index+2] = 's';
	temp_name[name_set_index+3] = 'v';
	FILE* outputfile = fopen(temp_name,"w");*/
	FILE* outputfile = stdout;

	//superblock summary
    int superblock_offset = 1024;
    struct ext2_super_block superblock;
	void* tempsbp=(void*)&superblock;
	pread(FSimagefd, tempsbp, sizeof(superblock), superblock_offset);
	unsigned int blockSize = 1024 << superblock.s_log_block_size;
	fprintf(outputfile, "%s,%d,%d,%d,%d,%d,%d,%d\n", "SUPERBLOCK", superblock.s_blocks_count, superblock.s_inodes_count,
		blockSize, superblock.s_inode_size, superblock.s_blocks_per_group, superblock.s_inodes_per_group,
		superblock.s_first_ino);

	//group summary
	int group_offset = 2048;
	int group_number = 0;
	struct ext2_group_desc groupdesc;
	void* tempgdp=(void*)&groupdesc;
	//superblock.s_blocks_per_group=1;
	unsigned int cur_blocks_pgroup = (superblock.s_blocks_count%superblock.s_blocks_per_group);
	pread(FSimagefd, tempgdp, sizeof(groupdesc), group_offset);
		fprintf(outputfile, "%s,%d,%d,%d,%d,%d,%d,%d,%d\n", "GROUP", group_number, cur_blocks_pgroup,
		superblock.s_inodes_per_group, groupdesc.bg_free_blocks_count, groupdesc.bg_free_inodes_count,
		groupdesc.bg_block_bitmap, groupdesc.bg_inode_bitmap, groupdesc.bg_inode_table);

	//free block entries
	unsigned char iscsize;
	unsigned int byte_index;
	for(byte_index = 0; byte_index < blockSize; byte_index++)
	{
		pread(FSimagefd, &iscsize, 1, groupdesc.bg_block_bitmap*blockSize + byte_index);
		int bit_index;
		for(bit_index = 0; bit_index < 8; bit_index++)
		{
			if((iscsize & (1 << bit_index)) == 0)
			{
				fprintf(outputfile, "BFREE,%d\n",(byte_index*8)+(bit_index+1));
			}
		}
	}

	//free I-node entries
	for(byte_index = 0; byte_index < blockSize; byte_index++)
	{
		pread(FSimagefd, &iscsize, 1, groupdesc.bg_inode_bitmap*blockSize + byte_index);
		int bit_index;
		for(bit_index = 0; bit_index < 8; bit_index++)
		{
			if((iscsize & (1 << bit_index)) == 0)
			{
				fprintf(outputfile, "IFREE,%d\n",(byte_index*8)+(bit_index+1));
			}
		}
	}

    //I-node summary
    int inodetable_offset = blockSize*groupdesc.bg_inode_table;
    struct ext2_inode inode;
    void* tempinp=(void*)&inode;
    unsigned int inode_index;
    for (inode_index = 0; inode_index < superblock.s_inodes_count; inode_index++)
    {
	    pread(FSimagefd, tempinp, sizeof(inode), inodetable_offset+(inode_index*sizeof(inode)));
        if (inode.i_mode != 0 && inode.i_links_count != 0)
        {
            fprintf(outputfile, "INODE,%d,",(inode_index+1));
            char filetype = '?';
            if (inode.i_mode & 0x8000)
            {
                filetype = 'f';
            }
            else if (inode.i_mode & 0x4000)
            {
                filetype = 'd';
            }
            else if (inode.i_mode & 0xA000)
            {
                filetype = 's';
            }

            char cTimeString[20];
            time_t ctime_stamp = inode.i_ctime;
        	struct tm cts = *gmtime(&ctime_stamp);
        	strftime(cTimeString, 80, "%m/%d/%y %H:%M:%S", &cts);

        	char mTimeString[20];
            time_t mtime_stamp = inode.i_mtime;
        	struct tm mts = *gmtime(&mtime_stamp);
        	strftime(mTimeString, 80, "%m/%d/%y %H:%M:%S", &mts);

            char aTimeString[20];
            time_t atime_stamp = inode.i_atime;
        	struct tm ats = *gmtime(&atime_stamp);
        	strftime(aTimeString, 80, "%m/%d/%y %H:%M:%S", &ats);

            fprintf(outputfile, "%c,%o,%d,%d,%d,%s,%s,%s,%d,%d",
             filetype, inode.i_mode & 0xFFF, inode.i_uid, inode.i_gid, inode.i_links_count,
            cTimeString, mTimeString, aTimeString, inode.i_size, inode.i_blocks);

            int num_blocks = EXT2_N_BLOCKS;
            int block_index;
            for (block_index = 0; block_index < num_blocks; block_index++)
            {
                fprintf(outputfile, ",%d", inode.i_block[block_index]);
            }
            fprintf(outputfile, "\n");

            //directory entries
            if (filetype == 'd' || filetype == 'f')
            {
                if (filetype == 'd')
                {
                    for (block_index = 0; block_index < num_blocks; block_index++)
                    {
                        if (inode.i_block[block_index] == 0)
                        {
                            break;
                        }
                        int direntry_offset = inode.i_block[block_index]*blockSize;
                        unsigned int dir_entry_byte_offset = 0;
                        struct ext2_dir_entry direntry;
                    	void* tempdep=(void*)&direntry;
                    	while(dir_entry_byte_offset < blockSize)
                    	{
                        	pread(FSimagefd, tempdep, sizeof(direntry), direntry_offset+dir_entry_byte_offset);
                        	if (direntry.inode != 0)
                        	{
                        	     fprintf(outputfile,"DIRENT,%d,%d,%d,%d,%d,'%s'\n",(inode_index+1),dir_entry_byte_offset,
                        	        direntry.inode, direntry.rec_len, direntry.name_len, direntry.name);
                        	}
                        	dir_entry_byte_offset += direntry.rec_len;
                    	}
                    }
                }

                //indirect block references
                int numindirBlocks = (blockSize/4);

            	int* singleIndirectPTR = malloc(blockSize);
                if(inode.i_block[12] > 0)
                {
                    pread(FSimagefd, singleIndirectPTR, blockSize, inode.i_block[12]*blockSize);
                    int indirb_index;
                    for(indirb_index = 0; indirb_index < numindirBlocks; indirb_index++)
                    {
                        if(singleIndirectPTR[indirb_index]==0)
                        {
                            break;
                        }
                        int direntry_offset = singleIndirectPTR[indirb_index] * blockSize;
                        unsigned int dir_entry_byte_offset = 0;
                        struct ext2_dir_entry direntry;
                    	void* tempdep=(void*)&direntry;
                        while (dir_entry_byte_offset < blockSize)
                        {
                            pread(FSimagefd, tempdep, sizeof(direntry), direntry_offset+dir_entry_byte_offset);
                            if (direntry.inode != 0)
                        	{
                                fprintf(outputfile, "INDIRECT,%d,%d,%d,%d,%d\n",(inode_index+1),1,(12+indirb_index),
                                    inode.i_block[12], inode.i_block[12]+indirb_index+1);
                        	}
                            dir_entry_byte_offset += direntry.rec_len;
                        }
                    }
                }

                int* doubleIndirectPTR = malloc(blockSize);
                if(inode.i_block[13] > 0)
                {
                    pread(FSimagefd, doubleIndirectPTR, blockSize, inode.i_block[13]*blockSize);
                    int indirb_index;
                    for(indirb_index = 0; indirb_index < numindirBlocks; indirb_index++)
                    {
                        if(doubleIndirectPTR[indirb_index]==0)
                        {
                            break;
                        }

                        int* singleIndirectPTR = malloc(blockSize);
                        pread(FSimagefd, singleIndirectPTR, blockSize, doubleIndirectPTR[indirb_index]*blockSize);

                        int indirbl2_index;
                        for(indirbl2_index = 0; indirbl2_index < numindirBlocks; indirbl2_index++)
                        {
                            if(singleIndirectPTR[indirbl2_index]==0)
                            {
                               break;
                            }

                            int direntry_offset = singleIndirectPTR[indirbl2_index]*blockSize;
                            unsigned int dir_entry_byte_offset = 0;
                            struct ext2_dir_entry direntry;
                        	void* tempdep=(void*)&direntry;
                            while (dir_entry_byte_offset < blockSize)
                            {
                                pread(FSimagefd, tempdep, sizeof(direntry), direntry_offset+dir_entry_byte_offset);
                                if (direntry.inode != 0)
                            	{
                                    fprintf(outputfile, "INDIRECT,%d,%d,%d,%d,%d\n",(inode_index+1),2,(13+indirbl2_index),
                                        inode.i_block[13], inode.i_block[13]+indirbl2_index+1);
                            	}
                                dir_entry_byte_offset += direntry.rec_len;
                            }
                        }
                    }
                }

                int* tripleIndirectPTR = malloc(blockSize);
                if(inode.i_block[14] > 0)
                {
                    pread(FSimagefd, singleIndirectPTR, blockSize, inode.i_block[14]*blockSize);
                    int numindirBlocks = (blockSize/4);
                    int indirb_index;
                    for(indirb_index = 0; indirb_index < numindirBlocks; indirb_index++)
                    {
                        pread(FSimagefd, doubleIndirectPTR, blockSize, singleIndirectPTR[indirb_index]*blockSize);

                        int indirbl2_index;
                        for(indirbl2_index = 0; indirbl2_index < numindirBlocks; indirbl2_index++)
                        {
                            pread(FSimagefd, tripleIndirectPTR, blockSize, doubleIndirectPTR[indirbl2_index]*blockSize);

                            int indirbl3_index;
                            for(indirbl3_index = 0; indirbl3_index < numindirBlocks; indirbl3_index++)
                            {
                                if(tripleIndirectPTR[indirbl3_index]==0)
                                {
                                    break;
                                }

                                int direntry_offset = tripleIndirectPTR[indirbl3_index] * blockSize;
                                unsigned int dir_entry_byte_offset = 0;
                                struct ext2_dir_entry direntry;
                            	void* tempdep=(void*)&direntry;
                                while (dir_entry_byte_offset < blockSize)
                                {
                                    pread(FSimagefd, tempdep, sizeof(direntry), direntry_offset+dir_entry_byte_offset);
                                    if (direntry.inode != 0)
                                	{
                                        fprintf(outputfile, "INDIRECT,%d,%d,%d,%d,%d\n",(inode_index+1),(14+indirbl3_index),2,
                                            inode.i_block[14], inode.i_block[14]+indirbl3_index+1);
                                	}
                                    dir_entry_byte_offset += direntry.rec_len;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
