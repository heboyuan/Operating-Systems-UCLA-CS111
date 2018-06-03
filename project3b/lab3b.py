import sys
import csv


class SuperBlock:
    def __init__(self, nblocks, ninodes, bsize, isize, bgroup, igroup, finode):
        self.num_blocks = nblocks
        self.num_inodes = ninodes
        self.block_size = bsize
        self.inode_size = isize
        self.blocks_per_group = bgroup
        self.indoes_per_group = igroup
        self.first_nonreserved_inode = finode


class Group:
    def __init__(self, gnumber):

class Inode:
    def __init__(self, inumber, ftype, mode, owner, group, lcount, ctime, mtime, atime, fsize, dspace, baddress):
        self.inode_number = inumber
        self.file_type = ftype
        self.mode = mode
        self.owner = owner
        self.group = group
        self.linkage_counts = lcount
        self.change_time = ctime
        self.modify_time = mtime
        self.access_time = atime
        self.file_size = fsize
        self.disk_space = dspace
        self.block_address = baddress


class DirectoryEntry:
    def __init__(self, pinode, loffset, rinode, elength, nlength, name):
        self.parent_inode = pinode
        self.logical_offset = loffset
        self.entry_reference_inode = rinode
        self.entry_length = elength
        self.entry_name_lenght = nlength
        self.entry_name = name


class Indirect:
    def __init__(self, pinumber, level, loffset, bnumber, rnumber):
        self.parent_inode_number = pinumber
        self.level = level
        self.logical_offset = loffset
        self.block_number = bnumber
        self.reference_number = rnumber


def main():
    # ================================================
    # Initialization
    # ================================================
    my_bfree = []
    my_ifree = []
    my_inode = []
    my_dir = []
    my_indir = []

    # ================================================
    # Parsing Arguments
    # ================================================
    if len(sys.argv) != 2:
        sys.stderr.write("Error: Unrecognized arguement")
        sys.exit(1)

    my_csv = open(sys.argv[1], 'r')
    for lines in my_csv:
        line_list = lines.split(",")
        if line_list[0] == "SUPERBLOCK":
            my_sp = SuperBlock(int(line_list[1]), int(line_list[2]), int(line_list[3]), int(line_list[4]),
                               int(line_list[5]), int(line_list[6]), int(line_list[7]))
        elif line_list[0] == "GROUP":
            my_group = Group(int(line_list[1]), int(line_list[2]), int(line_list[3]), int(line_list[4]),
                             int(line_list[5]), int(line_list[6]), int(line_list[7]), int(line_list[8]))
        elif line_list[0] == "BFREE":
            my_bfree.append(int(line_list[1]))
        elif line_list[0] == "IFREE":
            my_ifree.append(int(line_list[1]))
        elif line_list[0] == "INODE":
            my_inode.append(
                Inode(int(line_list[1]), line_list[2], int(line_list[3]), int(line_list[4]), int(line_list[5]),
                      int(line_list[6]), line_list[7], line_list[8], line_list[9], int(line_list[10]),
                      int(line_list[11]), list(map(int, line_list[12:]))))
        elif line_list[0] == "DIRENT":
            my_dir.append(
                DirectoryEntry(int(line_list[1]), int(line_list[2]), int(line_list[3]), int(line_list[4]),
                               int(line_list[5]),
                               line_list[6]))
        elif line_list[0] == "INDIRECT":
            my_indir.append(
                Indirect(int(line_list[1]), int(line_list[2]), int(line_list[3]), int(line_list[4]), int(line_list[5])))
        else:
            sys.stderr.write("Error: Worry data content\n")
            sys.exit(1)

    # =======================================================
    # Audit Block
    # =======================================================
    start_block = my_group.first_block_inode + int(my_sp.inode_size*my_group.iid_group/my_sp.block_size)
    for cur_inode in my_inode:
    	for index, ele in enumerate(cur_inode.block_address):
    		if ele != 0:
    			if index == 12:
    				cur_level = "INDIRECT BLOCK"
    				cur_offset = 12
    			elif index == 13:
    				cur_level = "DOUBLE INDIRECT BLOCK"
    				cur_offset = 268
    			elif index == 14:
    				cur_level = "TRIPLE INDIRECT BLOCK"
    				cur_offset = 65804
    			else:
    				cur_level = "BLOCK"
    				cur_offset = 0

    			if ele in my_bfree:
    				my_error = True
    				print('ALLOCATED BLOCK {} ON FREELIST'.format(ele))
    			else:
    				changed = False
    				if ele < 0 or (ele > my_sp.num_blocks and cur_inode.file_type != 's'):
    					print('INVALID {} {} IN INODE {} AT OFFSET {}'.format(cur_level, ele, cur_inode.inode_number, cur_offset))
    				else:
    					changed = True
    					if ele in block_dic:
    						block_dic[ele].append([cur_level, ele, cur_inode.inode_number, cur_offset])
    					else:
    						block_dic[ele] = []
    						block_dic[ele].append([cur_level, ele, cur_inode.inode_number, cur_offset])
    				if ele < start_block:
    					print('RESERVED {} {} IN INODE {} AT OFFSET {}'.format(cur_level, ele, cur_inode.inode_number, cur_offset))
    				elif changed == False:
    					if ele in block_dic:
    						block_dic[ele].append([cur_level, ele, cur_inode.inode_number, cur_offset])
    					else:
    						block_dic[ele] = []
    						block_dic[ele].append([cur_level, ele, cur_inode.inode_number, cur_offset])

if __name__ == "__main__":
    main()
