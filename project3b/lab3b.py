import sys
import csv


class SuperBlock:
    def __init__(self, nblocks):
        self.num_blocks = nblocks


def main():
    #================================================
    #Initialization
    #================================================
    my_bfree = []
    my_ifree = []
    my_inode = []
    my_dir = []
    my_indir = []

    #================================================
    #Parsing Arguements
    #================================================
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
            my_inode.append(inode(int(line_list[1]),line_list[2],int(line_list[3]),int(line_list[4]),int(line_list[5]),int(line_list[6]),line_list[7],line_list[8],line_list[9],int(line_list[10]),int(line_list[11]),list(map(int,line_list[12:]))))
        elif line_list[0] == "DIRENT":
        	my_dir.append(dir(int(line_list[1]),int(line_list[2]),int(line_list[3]),int(line_list[4]),int(line_list[5]),line_list[6]))
        elif line_list[0] == "INDIRECT":
        	my_indir.append(indir(int(line_list[1]),int(line_list[2]),int(line_list[3]),int(line_list[4]),int(line_list[5])))
        else:
        	sys.stderr.write("Error: Worry data content\n")
        	sys.exit(1)

if __name__ == "__main__":
    main()
