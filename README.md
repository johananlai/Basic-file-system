# File System
This is a practice project written in C that simulates a real file system according to certain specifications listed below. Each file in the system consists only of `char`s.

## Specification
The file system contains a disk that grows dynamically as files are added and written to. It can create and open files, read and write from open files, and close and remove existing files. It should also be able to be saved to a text file, and initiate the disk from a text file.

The file system contains 64 blocks that are 16 ints wide each. It reserves the first few blocks of the disk for a bitmap, file descriptors, and the file directory.

In addition to these reserved blocks, the system also maintains an Open File Table which has a maximum of 4 slots, the first of which is reserved for the directory, which is created like any other file.

File descriptors should be 4 ints wide; the first specifies the file length, while the next 3 specify the indices of the blocks that belong to the file.

The OFT maintains information about the file, including a read/write buffer, its current file position, and its file descriptor.

## Getting started
### Usage
Compile the file with `gcc` and run the executable in a shell.
```sh
gcc file_sys.c -o file_sys
./file_sys
```

### I/O Commands
To interact with the file system, use the following input:
* `cr <filename>` - Creates a new file with the name <filename>
* `de <filename>` - Destroy the file <filename>
* `op <filename>` - Open the file <filename> for reading and writing. Print the OFT index it was opened in
* `cl <index>` - Close the specified file at OFT index <index>
* `rd <index> <count>` - Sequentially read <count> number of characters starting at the file's current position from the specified OFT index <index> and print them out
* `wr <index> <char> <count>` - Sequentially write <count> number of <char>s into the specified file <index> starting at the file's current position
* `sk <index> <pos>` - Seek; set the current position of the specified OFT index <index> to <pos>
* `ls` - Directory; list the names of all files in the system
* `in <disk.txt>` - Initialize a disk from the specified text file and create and open the directory file. (Re)initialize an empty disk if no file name is specified, and close all open files in the OFT.
* `sv <disk.txt>` - Close all files and save the contents of the disk in the specified text file.
* `quit` - Quits the program.

