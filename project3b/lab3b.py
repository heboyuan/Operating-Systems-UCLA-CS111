import sys
import csv


class SuperBlock:
    def __init__(self, nblocks):
        self.num_blocks = nblocks


def main():
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
            my_inode.append()


if __name__ == "__main__":
    main()
