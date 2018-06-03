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
        self.first_inode = finode


class Group:
    def __init__(self, gid, bingroup, iingroup, fbid, fiid, fbbn, fibn, fbinode):
        self.group_id = gid
        self.bid_group = bingroup
        self.iid_group = iingroup
        self.freeblock_id = fbid
        self.freeinode_id = fiid
        self.freeblock_bitmap_id = fbbn
        self.freeinode_bitmap_id = fibn
        self.first_block_inode = fbinode


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
        self.entry_name_length = nlength
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
    my_error = False

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
    block_dic = {}
    start_block = my_group.first_block_inode + int(my_sp.inode_size * my_group.iid_group / my_sp.block_size)
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
                        my_error = True
                        print('INVALID {} {} IN INODE {} AT OFFSET {}'.format(cur_level, ele, cur_inode.inode_number,
                                                                              cur_offset))
                    else:
                        changed = True
                        if ele in block_dic:
                            block_dic[ele].append([cur_level, ele, cur_inode.inode_number, cur_offset])
                        else:
                            block_dic[ele] = []
                            block_dic[ele].append([cur_level, ele, cur_inode.inode_number, cur_offset])
                    if ele < start_block:
                        my_error = True
                        print('RESERVED {} {} IN INODE {} AT OFFSET {}'.format(cur_level, ele, cur_inode.inode_number,
                                                                               cur_offset))
                    elif not changed:
                        if ele in block_dic:
                            block_dic[ele].append([cur_level, ele, cur_inode.inode_number, cur_offset])
                        else:
                            block_dic[ele] = [[cur_level, ele, cur_inode.inode_number, cur_offset]]

    for cur_indir in my_indir:
        if cur_indir.level == 1:
            cur_level = "INDIRECT BLOCK"
        elif cur_indir.level == 2:
            cur_level = "DOUBLE INDIRECT BLOCK"
        elif cur_indir.level == 3:
            cur_level = "TRIPLE INDIRECT BLOCK"
        else:
            cur_level = "BLOCK"

        if cur_indir.reference_number in my_bfree:
            my_error = True
            print('ALLOCATED BLOCK {} ON FREELIST'.format(cur_indir.reference_number))
        else:
            if cur_indir.reference_number < 0 or cur_indir.reference_number > my_sp.num_blocks:
                my_error = True
                print('INVALID {} {} IN INODE {} AT OFFSET {}'.format(cur_level, cur_indir.reference_number,
                                                                      cur_indir.parent_inode_number,
                                                                      cur_indir.logical_offset))
            if cur_indir.reference_number < start_block:
                my_error = True
                print('RESERVED {} {} IN INODE {} AT OFFSET {}'.format(cur_level, cur_indir.reference_number,
                                                                       cur_indir.parent_inode_number,
                                                                       cur_indir.logical_offset))
            if cur_indir.reference_number in block_dic:
                block_dic[cur_indir.reference_number].append(
                    [cur_level, cur_indir.reference_number, cur_indir.parent_inode_number, cur_indir.logical_offset])
            else:
                block_dic[cur_indir.reference_number] = [
                    [cur_level, cur_indir.reference_number, cur_indir.parent_inode_number, cur_indir.logical_offset]]

    for _, value in block_dic.items():
        if len(value) > 1:
            for temp_block in value:
                print('DUPLICATE {} {} IN INODE {} AT OFFSET {}'.format(temp_block[0], temp_block[1], temp_block[2],
                                                                        temp_block[3]))

    for temp_add in range(start_block, my_group.bid_group):
        if temp_add not in my_bfree and temp_add not in block_dic:
            my_error = True
            print('UNREFERENCED BLOCK {}'.format(temp_add))

    # =======================================================
    # Audit Inode
    # =======================================================
    my_unallocated_inodes = my_ifree
    my_allocated_inodes = []
    my_reserved = []

    for inode in my_inode:
        if inode.file_type == '0':
            if inode.inode_number not in my_ifree:
                print("UNALLOCATED INODE {} NOT ON FREELIST".format(inode.inode_number))
                my_error = True
                my_unallocated_inodes.append(inode.inode_number)
            else:
                if inode.inode_number in my_ifree:
                    print("ALLOCATED INODE {} ON FREELIST".format(inode.inode_number))
                    my_error = True
                    my_unallocated_inodes.remove(inode.inode_number)

                my_allocated_inodes.append(inode.inode_number)

    for i_pos in range(my_sp.first_inode, my_sp.num_inodes):
        for inode in my_inode:
            if inode.inode_number == i_pos:
                my_reserved.append(inode)
        if not (not (len(my_reserved) <= 0) or not (i_pos not in my_ifree)):
            # De Morgan Law changed, check if there is an error
            print("UNALLOCATED INODE {} NOT ON FREELIST".format(i_pos))
            my_error = True
            my_unallocated_inodes.append(i_pos)

    # =======================================================
    # Audit Directory
    # =======================================================
    my_parent_inode = []
    my_dict_inode = {}

    for directoryentry in my_dir:
        inode_number = directoryentry.entry_reference_inode
        if directoryentry.entry_name != "'.'" and directoryentry.name != "'..'":
            if 1 <= inode_number <= my_sp.num_inodes and (inode_number not in my_unallocated_inodes):
                my_parent_inode[inode_number] = directoryentry.parent_inode
    my_parent_inode[2] = 2

    for directoryentry in my_dir:
        inode_number = directoryentry.entry_reference_inode
        directory_name = directoryentry.name[:-1]
        parent_inode_number = directoryentry.parent_inode
        if inode_number in my_unallocated_inodes:
            print('DIRECTORY INODE {} NAME {} UNALLOCATED INODE {}'.format(parent_inode_number, directory_name,
                                                                           inode_number))
            my_error = True
        elif 1 < inode_number < my_sp.num_inodes:
            print(
                'DIRECTORY INODE {} NAME {} INVALID INODE {}'.format(parent_inode_number, directory_name, inode_number))
            my_error = True
        else:
            my_dict_inode[inode_number] = my_dict_inode.get(inode_number, 0) + 1

        if directory_name == "'.'":
            if inode_number != parent_inode_number:
                print("DIRECTORY INODE {} NAME '.' LINK TO INODE {} SHOULD BE {}".format(parent_inode_number,
                                                                                         inode_number,
                                                                                         parent_inode_number))
                my_error = True
        elif directory_name == "'..'":
            if my_parent_inode[parent_inode_number] != parent_inode_number:  # Check this !!!!!!!!!!!
                print("DIRECTORY INODE {} NAME '..' LINK TO INODE {} SHOULD BE {}".format(inode_number,
                                                                                          parent_inode_number,
                                                                                          my_parent_inode[
                                                                                              parent_inode_number]))

    for inode in my_allocated_inodes:
        if inode.inode_number not in my_dict_inode:
            if inode.linkage_counts != 0:
                my_error = True
                print('INODE {} HAS 0 LINKS BUT LINKCOUNT IS {}'.format(inode.inode_number, inode.link_count))
        else:
            temp_link_inode = inode.linkage_counts
            temp_link_entry = my_dict_inode[inode.inode_number]
            if temp_link_entry != temp_link_inode:
                print('INODE {} HAS {} LINKS BUT LINKCOUNT IS {}'.format(inode.inode_number, temp_link_entry,
                                                                         temp_link_inode))
                my_error = True

    # =======================================================
    # Error Checking
    # =======================================================
    sys.exit(2) if my_error else sys.exit(0)


if __name__ == "__main__":
    main()
