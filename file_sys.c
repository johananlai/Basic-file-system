#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned int const blockSize = 64;
char* ldisk;
char* frontCache;
char* OFT;


void read_block(int i, char* p)
{
    memcpy(p, (ldisk + i*blockSize),  blockSize);
}

void write_block(int i, char* p)
{
    memcpy((ldisk + i*blockSize), p, blockSize);
}

void init_ldisk()
{
    // Initialize ldisk & frontCache (bitmap & fds)
    ldisk = (char*) malloc(16*sizeof(int) * blockSize);
    frontCache = (char*) malloc(7*sizeof(char) * blockSize);
    OFT = (char*) malloc(4*(blockSize + 2*sizeof(int)));
    
    int i;
    for (i = 0; i < 10; ++i)
        *(frontCache + i) |= 1;

    for (i = 0; i < 24; ++i)
        *(frontCache + blockSize + i*4*sizeof(int)) = -1;

    for (i = 0; i < 7; ++i)
        write_block(i, (frontCache + i*blockSize));

    // Set directory block #s
    *(frontCache + blockSize) = 0;            
    for (i = 0; i < 3; ++i)
        *(frontCache + blockSize + (i+1)*sizeof(int)) = i+7;

    write_block(1, (char*) (frontCache + blockSize));

    // Initialize OFT
    for (i = 0; i < 4; ++i)
        *(OFT + blockSize + sizeof(int) + i*(blockSize + 2*sizeof(int))) = -1;
}

void create(char* symbolic_file_name)
{
    // Find fd
    int fd = 0;
    while (fd < 24 && *(frontCache + blockSize + fd*4*sizeof(int)) != -1)
        fd++;

    // Find directory entry
    char* directory = (char*) malloc(3*blockSize);
    int i;
    for (i = 0; i < 3; ++i)
        read_block(i+7, (directory + i*blockSize));

    int fileEntryFound = 0;
    for (i = 0; i < 24; ++i)
    {
        char fileName[4];
        memcpy(&fileName, (directory + i*2*sizeof(int)), 4);
        
        if (strncmp(symbolic_file_name, fileName, 4) == 0)
        {
            printf("Error: File with this name already exists!\n");
            return;
        }
        if (!fileEntryFound && *(directory + (i*2+1)*sizeof(int)) == 0)
        {
            fileEntryFound = 1;
            memcpy((directory + i*2*sizeof(int)), symbolic_file_name, 4);
            *(directory + (i*2+1)*sizeof(int)) = fd;
        }
    }

    // Update frontCache and write to ldisk
    *(frontCache + blockSize + fd*4*sizeof(int)) = 0;

    int j = 0;
    while ((*(frontCache + j) & 0x1) == 1)
        j++;

    *(frontCache + j) |= 1;
    *(frontCache + blockSize + (4*fd+1)*sizeof(int)) = j;

    for (i = 0; i < 7; ++i)
        write_block(i, (char*) (frontCache + i*blockSize));

    for (i = 0; i < 3; ++i)
        write_block(i+7, (char*) (directory + i*blockSize));

    printf("%s created\n", symbolic_file_name);
}

void destroy(char* symbolic_file_name)
{
    // Read directory
    int fd = 0;
    char* directory = (char*) malloc(3*blockSize);

    int i;
    for (i = 0; i < 3; ++i)
        read_block(i+7, (directory + i*blockSize));

    // Check if name matches and find fd
    for (i = 0; i < 24; ++i)
    {
        char fileName[4];
        memcpy(&fileName, (directory + i*2*sizeof(int)), 4);
        
        if (strncmp(symbolic_file_name, fileName, 4) == 0)
        {
            // Clear directory entry
            *(directory + i*2*sizeof(int)) = 0;
            *(directory + (i*2+1)*sizeof(int)) = 0; 

            // Update frontCache and write to ldisk
            fd = i+1;
            *(frontCache + blockSize + fd*4*sizeof(int)) = -1;
            int j;
            for (j = 1; j < 4; ++j)
            {
                int blockNum;
                memcpy(&blockNum, (frontCache + blockSize + (fd*4+j)*sizeof(int)), 4);
                if (blockNum != 0)
                    *(frontCache + blockNum) |= 0;
            }
                                                                       
            for (i = 0; i < 7; ++i)
                write_block(i, (char*) (frontCache + i*blockSize));
                                                             
            for (i = 0; i < 3; ++i)
                write_block(i+7, (char*) (directory + i*blockSize));

            printf("%s destroyed\n", symbolic_file_name);
            return;
        }
    }
    printf("Error: File was not found\n");
}

int open(char* symbolic_file_name)
{
    // Read directory
    int fd;
    char* directory = (char*) malloc(3*blockSize);

    int i;
    for (i = 0; i < 3; ++i)
        read_block(i+7, (directory + i*blockSize));

    // Check if name matches and find fd
    for (i = 0; i < 24; ++i)
    {
        char fileName[4];
        memcpy(&fileName, (directory + i*2*sizeof(int)), 4);
        
        if (strncmp(symbolic_file_name, fileName, 4) == 0)
        {
            fd = *(directory + (i*2+1)*sizeof(int));

            int j;
            for (j = 1; j < 4; ++j)
            {
                if (*(OFT + blockSize + sizeof(int) + j*(blockSize + 2*sizeof(int))) == -1)
                {
                    // Read first block into buffer, set file pos, set fd
                    read_block(*(frontCache + blockSize + (fd*4+1)*sizeof(int)),
                               OFT + j*(blockSize + 2*sizeof(int)));
                    *(OFT + blockSize + j*(blockSize + 2*sizeof(int))) = 0;
                    *(OFT + blockSize + sizeof(int) + j*(blockSize + 2*sizeof(int))) = fd;
                    
                    printf("%s opened %d\n", symbolic_file_name, j);
                    return j;
                }
            }
            printf("Error: OFT is full\n");
            return -1;
        }
    }
    printf("Error: File was not found\n");
    return -2;
}

void close(int index)
{
    if (index < 0 || index > 3)
    {
        printf("Error: File with this OFT index was not found\n");
        return;
    }

    // Write to disk and update file size
    int fd = *(OFT + blockSize + sizeof(int) + index*(blockSize + 2*sizeof(int)));
    write_block(*(OFT + blockSize + index*(blockSize + 2*sizeof(int))),
                OFT + index*(blockSize + 2*sizeof(int)));
    *(frontCache + blockSize + fd*4*sizeof(int)) += blockSize;

    int i;
    for (i = 0; i < 7; ++i)
        write_block(i, (frontCache + i*blockSize));

    // Clear OFT entry
    *(OFT + blockSize + sizeof(int) + index*(blockSize + 2*sizeof(int))) = -1;

    printf("%d closed\n", index);
}

void read(int index, char* mem_area, int count)
{
    if (index < 0 || index > 3)
    {
        printf("Error: File with this OFT index was not found\n");
        return;
    }

    // Update OFT buffer
    int fd = *(OFT + blockSize + sizeof(int) + index*(blockSize + 2*sizeof(int)));
    read_block(*(frontCache + blockSize + (fd*4+1)*sizeof(int)),
               OFT + index*(blockSize + 2*sizeof(int)));
    int bufPos = *(OFT + blockSize + index*(blockSize + 2*sizeof(int)));
   
    int i;
    int bytesRead = 0;
    int nextBlock = 2;
    for (i = bufPos; bytesRead < count; ++i, ++bytesRead)
    {
        // If end of buffer reached
        if (i >= blockSize)
        {
            i = i % blockSize;
            read_block(*(frontCache + blockSize +(fd*4+nextBlock)*sizeof(int)),
                       OFT + index*(blockSize + 2*sizeof(int)));
            nextBlock++;
        }
        memcpy((mem_area + bytesRead), (OFT + index*(blockSize + 2*sizeof(int)) + i), 1);
    }
}

void write(int index, char* mem_area, int count)
{
    if (index < 0 || index > 3)
    {
        printf("Erorr: File with this OFT index was not found\n");
        return;
    }
    
    // Calculate buffer file pos, current block of file being written
    int fd = *(OFT + blockSize + sizeof(int) + index*(blockSize + 2*sizeof(int)));
    int bufPos = *(frontCache + blockSize + fd*4*sizeof(int)) % blockSize;
   
    int i;
    int currBlock = (int) (*(OFT + blockSize + index*(blockSize + 2*sizeof(int)))/blockSize) + 1;
    int bytesWritten = 0;
    for (i = bufPos; bytesWritten < count; ++i, ++bytesWritten)
    {
        // If end of buffer reached
        if (i == blockSize)
        {
            i = 0;
            write_block(*(frontCache + blockSize + (fd*4+currBlock)*sizeof(int)),
                        OFT + index*(blockSize + 2*sizeof(int)));

            // Clear OFT buffer
            int j;
            for (j = 0; j < blockSize; ++j)
                *(OFT + index*(blockSize + 2*sizeof(int)) + j) &= 0;

            currBlock++;
            
            // Update bitmap, fd block num
            j = 0;
            while ((*(frontCache + j) & 0x1) == 1)
                j++;

            *(frontCache + j) |= 1;
            *(frontCache + blockSize + (fd*4+currBlock)*sizeof(int)) = j;
        }

        memcpy((OFT + index*(blockSize + 2*sizeof(int)) + i), (mem_area + bytesWritten), 1);
    }

    // Update buffer file pos, write from buffer to disk, update file size
    *(OFT + blockSize + index*(blockSize + 2*sizeof(int))) += bytesWritten;
    write_block(*(frontCache + blockSize + (fd*4+currBlock)*sizeof(int)),
                OFT + index*(blockSize + 2*sizeof(int)));

    *(frontCache + blockSize + fd*4*sizeof(int)) += bytesWritten;

    for (i = 0; i < 7; ++i)                          
        write_block(i, (frontCache + i*blockSize));

    printf("%d bytes written\n", count);    
}

void lseek(int index, int pos)
{
    if (index < 0 || index > 3)
    {
        printf("Error: File with this OFT index was not found\n");
        return;
    }

    // If pos is outside of current block
    int currBlock = (int) pos/blockSize;
    if (currBlock > 0)
    {
        int fd = *(OFT + blockSize + sizeof(int) + index*(blockSize + 2*sizeof(int)));
        // If pos is out of bounds
        if (pos >= *(frontCache + blockSize + fd*4*sizeof(int)))
        {
            printf("Error: Position is out of bounds\n");
            return;
        }

        read_block(*(frontCache + blockSize + (fd*4+currBlock+1)*sizeof(int)),
                   OFT + index*(blockSize + 2*sizeof(int)));
    }

    *(OFT + blockSize + index*(blockSize + 2*sizeof(int))) = pos;
    printf("position is %d\n", pos);
}

void directory()
{
    char* directory = (char*) malloc(3*blockSize);

    int i;
    for (i = 0; i < 3; ++i)
        read_block(i+7, (directory + i*blockSize));

    for (i = 0; i < 24; ++i)
    {
        char fileName[4];
        memcpy(&fileName, (directory + i*2*sizeof(int)), 4);
        
        if (*(directory + (i*2+1)*sizeof(int)) != 0)
            printf("%s ", fileName);
    }
    printf("\n");
}

void init(char* file_name)
{
    FILE* fp;
    if (file_name)
    {
        fp = fopen(file_name, "rb");
    }

    if (file_name && fp != NULL)
    {
        char buf[blockSize];
        int i;
        for (i = 0; i < 16*sizeof(int); ++i)
        {
            fread(buf, sizeof(char), blockSize, fp);
            write_block(i, buf);
        }
        fclose(fp);

        // Initialize front cache from copied ldisk
        for (i = 0; i < 7; ++i)                               
            read_block(i, (frontCache + i*blockSize));

        printf("disk restored\n");
    }
    else
    {
        init_ldisk();
        printf("disk initialized\n");
    }
    
    // Open directory
    read_block(7, OFT);    
    *(OFT + blockSize) = 0;
    *(OFT + blockSize + sizeof(int)) = 0;
}

void save(char* file_name)
{
    FILE* fp;
    fp = fopen(file_name, "wb");
    
    char buf[blockSize];
    int i;
    for (i = 0; i < 16*sizeof(int); ++i)
    {
        read_block(i, buf);
        fwrite(buf, sizeof(char), blockSize, fp);
    }
    fclose(fp);

    // Close all entries in OFT
    for (i = 1; i < 4; ++i)
        *(OFT + blockSize + sizeof(int) + i*(blockSize + 2*sizeof(int))) = -1;

    printf("disk saved\n\n");
}


void run()
{
    // Get input
    char cmd[256];
    scanf(" %[^\n]", cmd);
    
    while (1)
    {
        // Format input
        strtok(cmd, " ");

        if (strncmp(cmd, "cr", 2) == 0)
        {
            char* name;
            name = strtok(NULL, " ");
            create(name);
        }
        else if (strncmp(cmd, "de", 2) == 0)
        {
            char* name;
            name = strtok(NULL, " ");
            destroy(name);
        }
        else if (strncmp(cmd, "op", 2) == 0)
        {
            char* name;
            name = strtok(NULL, " ");
            open(name);
        }
        else if (strncmp(cmd, "cl", 2) == 0)
        {
            char* index;
            index = strtok(NULL, " ");
            close(atoi(index));
        }
        else if (strncmp(cmd, "rd", 2) == 0)
        {
            char* ind;
            char* cnt;
            ind = strtok(NULL, " ");
            cnt = strtok(NULL, " ");
            int index = atoi(ind);
            int count = atoi(cnt);

            char* output = (char*) malloc(count*sizeof(char));
            read(index, output, count);

            int i;
            for (i = 0; i < count; ++i)
                printf("%c", *(output + i));
            printf("\n");
        }
        else if (strncmp(cmd, "wr", 2) == 0)
        {
            char* ind;
            char* ch;
            char* cnt;
            ind = strtok(NULL, " ");
            ch = strtok(NULL, " ");
            cnt = strtok(NULL, " ");
            int index = atoi(ind);
            int count = atoi(cnt);

            char* input = (char*) malloc(count*sizeof(char));
            int i;
            for (i = 0; i < count; ++i)
                *(input + i) = *ch;

            write(index, input, count);
        }
        else if (strncmp(cmd, "sk", 2) == 0)
        {
            char* ind;
            char* pos;
            ind = strtok(NULL, " ");
            pos = strtok(NULL, " ");
            int index = atoi(ind);
            int position = atoi(pos);

            lseek(index, position);
        }
        else if (strncmp(cmd, "ls", 2) == 0)
        {
            directory();
        }
        else if (strncmp(cmd, "in", 2) == 0)
        {
            char* file;
            file = strtok(NULL, " ");
            init(file);
        }
        else if (strncmp(cmd, "sv", 2) == 0)
        {
            char* file;
            file = strtok(NULL, " ");
            save(file);
        }
        else if (strncmp(cmd, "quit", 4) == 0)
        {
            break;
        }
        else
        {
            printf("Error: Invalid input\n");
        }

        strcpy(cmd, "");
        scanf(" %[^\n]", cmd);
    }
}


int main()
{
    run();
    return 0;
}
