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
    def __init__(self, pinode, loffset, rinode, elength, nlength):
        self.parent_inode = pinode


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
    # Parsing Arguements
    # ================================================
    if len(sys.argv) != 2:
        sys.stderr.write("Error: Unrecognized arguement")
        sys.exit(1)

    csv = open(sys.argv[1], 'r')
    for lines in csv:
        line_list = lines.split(",")
        if line_list[0] == "SUPERBLOCK":
            my_sp = SuperBlock(int(line_list[1]), int(line_list[2]), int(line_list[3]), int(line_list[4]),
                               int(line_list[5]), int(line_list[6]), int(line_list[7]))
        elif line_list[0] == "BFREE":
            my_bfree.append(int(line_list[1]))
        elif line_list[0] == "IFREE":
            my_ifree.append(int(line_list[1]))
        elif line_list[0] == "INODE":
            my_inode.append(
                inode(int(line_list[1]), line_list[2], int(line_list[3]), int(line_list[4]), int(line_list[5]),
                      int(line_list[6]), line_list[7], line_list[8], line_list[9], int(line_list[10]),
                      int(line_list[11]), list(map(int, line_list[12:]))))
        elif line_list[0] == "DIRENT":
            my_dir.append(
                dir(int(line_list[1]), int(line_list[2]), int(line_list[3]), int(line_list[4]), int(line_list[5]),
                    line_list[6]))
        elif line_list[0] == "INDIRECT":
            my_indir.append(
                indir(int(line_list[1]), int(line_list[2]), int(line_list[3]), int(line_list[4]), int(line_list[5])))
        else:
            sys.stderr.write("Error: Worry data content\n")
            sys.exit(1)


if __name__ == "__main__":
    main()
