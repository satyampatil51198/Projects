#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<string.h>
#include<iostream>

#define READ 1
#define WRITE 2

#define MAXFILESIZE 2048
#define MAXINODE 50

#define REGULAR 1
#define SPECIAL 2

#define START 0
#define END 2
#define CURRENT 1

typedef struct superblock
{
    int TotalInodes;
    int FreeInodes;
}SUPERBLOCK,* PSUPERBLOCK;


typedef struct inode
{
    char FileName[50];
    int InodeNumber;
    int FileSize;
    int FileActualSize;
    int FileType;
    char *Buffer;
    int LinkCount;
    int ReferenceCount;
    int Permission;
    struct inode * next;
}INODE, *PINODE, **PPINODE;


typedef struct FileTable
{
    int ReadOffset;
    int WriteOffset;
    int Count;
    int Mode;
    PINODE ptrinode;
}FILETABLE,* PFILETABLE;

typedef struct ufdt
{
    PFILETABLE ptrfiletable;
}UFDT;

UFDT UFDTarr[50];

SUPERBLOCK SUPERBLOCKobj;

PINODE Head=NULL;

void CreateDILB()
{
    int i=1;
    PINODE temp = Head;
    PINODE newn = NULL;

    while(i < MAXINODE)
    {
        newn = (PINODE)malloc(sizeof(INODE));

        newn->InodeNumber = i;
        newn->FileSize = 0;
        newn->FileType = 0;
        newn->LinkCount = 0;
        newn->ReferenceCount = 0;
        newn->next = NULL;
        newn->Buffer = NULL;

        if(temp == NULL)
        {
            Head = newn;
            temp = Head;
        }
        else
        {
            temp->next = newn;
            temp = temp->next;
        }

        i++;
    }
    printf("DILB created successfully\n");
}

void InitialiseSuperBlock()
{
    
    int i=0;
    while(i < MAXINODE)
    {
        UFDTarr[i].ptrfiletable = NULL;
        i++;
    }
    
    SUPERBLOCKobj.TotalInodes = MAXINODE;
    SUPERBLOCKobj.FreeInodes = MAXINODE;

    printf("SuperBlock initialised successfully\n");
}

int GetFDFromName(char *name)
{
    int i=0;

    while(i < MAXINODE)
    {
        if(UFDTarr[i].ptrfiletable != NULL)
        {
            if(strcmp((UFDTarr[i].ptrfiletable->ptrinode->FileName),name) == 0)
            {
                break;
            }
            i++;
        }
        if(i == 50)
        {
            return -1;
        }
        else
        {
            return i;
        }
        
    }
}

PINODE Get_Inode(char * name)
{
    PINODE temp = Head;
    int i=0;

    if(name == NULL)
    {
        return NULL;
    }
    
    while(temp != NULL)
    {
        if(strcmp(name,temp->FileName) == 0)
        {
            break;
        }
        temp = temp->next;
    }
    return temp;
}

int CreateFile(char *name,int permission)
{
    int i=0;

    PINODE temp = Head;

    if((name == NULL) || (permission == 0) || (permission > 3))
    {
        return -1;
    }

    if(SUPERBLOCKobj.FreeInodes == 0)
    {
        return -2;
    }

    (SUPERBLOCKobj.FreeInodes)--;

    if(Get_Inode(name) != NULL)
    {
        return -3;
    }

    while(temp != NULL)
    {
        if(temp->FileType == 0)
        {
            break;
        }
        temp = temp->next;
    }

    while(i<50)
    {
        if(UFDTarr[i].ptrfiletable == NULL)
        {
            break;
        }
        i++;
    }

    UFDTarr[i].ptrfiletable = (PFILETABLE)malloc(sizeof(FILETABLE));

    UFDTarr[i].ptrfiletable->ReadOffset = 0;
    UFDTarr[i].ptrfiletable->WriteOffset = 0;
    UFDTarr[i].ptrfiletable->Count = 1;
    UFDTarr[i].ptrfiletable->Mode = permission;
    
    UFDTarr[i].ptrfiletable->ptrinode = temp;

    strcpy(UFDTarr[i].ptrfiletable->ptrinode->FileName,name);

    UFDTarr[i].ptrfiletable->ptrinode->FileType = REGULAR;
    UFDTarr[i].ptrfiletable->ptrinode->ReferenceCount = 1;
    UFDTarr[i].ptrfiletable->ptrinode->LinkCount = 1;
    UFDTarr[i].ptrfiletable->ptrinode->FileSize = MAXFILESIZE;
    UFDTarr[i].ptrfiletable->ptrinode->FileActualSize = 0;
    UFDTarr[i].ptrfiletable->ptrinode->Permission = permission;

    UFDTarr[i].ptrfiletable->ptrinode->Buffer =(char*)malloc(MAXFILESIZE);

    return i;

}

int rm_File(char *name)
{
    int fd = 0;

    fd = GetFDFromName(name);

    if(fd == -1)
    {
        return -1;
    }

    (UFDTarr[fd].ptrfiletable->ptrinode->LinkCount)--;

    if(UFDTarr[fd].ptrfiletable->ptrinode->LinkCount == 0)
    {
        UFDTarr[fd].ptrfiletable->ptrinode->FileType = 0;
        free(UFDTarr[fd].ptrfiletable);
    }

    UFDTarr[fd].ptrfiletable = NULL;
    (SUPERBLOCKobj.FreeInodes)++;
}

void ls_file()
{
    int i = 0;

    PINODE temp = Head;

    if(SUPERBLOCKobj.FreeInodes == MAXINODE)
    {
        printf("There is no such a file\n");
        return;
    }

    printf("\nFile Name\tInode Number\tFile Size\tLink Count\n");

    printf("-------------------------------------------------------------------\n");

    while(temp != NULL)
    {
        if(temp->FileType != 0)
        {
            printf("%s\t\t%d\t\t%d\t\t%d\n",temp->FileName,temp->InodeNumber,temp->FileActualSize,temp->LinkCount);

        }
        temp = temp->next;
    }
    printf("\n------------------------------------------------------------------\n");
}

int WriteFile(int fd,char *arr,int isize)
{
    if(((UFDTarr[fd].ptrfiletable->Mode) != WRITE) && ((UFDTarr[fd].ptrfiletable->Mode) != READ + WRITE))
    {
        return -1;
    }

    if(((UFDTarr[fd].ptrfiletable->ptrinode->Permission) != WRITE) && ((UFDTarr[fd].ptrfiletable->ptrinode->Permission) != READ + WRITE))
    {
        return -1;
    }

    if((UFDTarr[fd].ptrfiletable->WriteOffset) == MAXFILESIZE)
    {
        return -2;
    }

    if((UFDTarr[fd].ptrfiletable->ptrinode->FileType) != REGULAR)
    {
        return -1;
    }

    strncpy((UFDTarr[fd].ptrfiletable->ptrinode->Buffer)+(UFDTarr[fd].ptrfiletable->WriteOffset),arr,isize);

    (UFDTarr[fd].ptrfiletable->WriteOffset) = (UFDTarr[fd].ptrfiletable->WriteOffset) + isize;

    (UFDTarr[fd].ptrfiletable->ptrinode->FileActualSize) = (UFDTarr[fd].ptrfiletable->ptrinode->FileActualSize) + isize;

    return isize;
}

int ReadFile(int fd,char *arr,int isize)
{
    int read_size = 0;

    if(UFDTarr[fd].ptrfiletable == NULL)
    {
        return -1;
    }

     if(((UFDTarr[fd].ptrfiletable->Mode) != READ) && ((UFDTarr[fd].ptrfiletable->Mode) != READ + WRITE))
    {
        return -2;
    }

    if(((UFDTarr[fd].ptrfiletable->ptrinode->Permission) != READ) && ((UFDTarr[fd].ptrfiletable->ptrinode->Permission) != READ + WRITE))
    {
        return -2;
    }

    if((UFDTarr[fd].ptrfiletable->ReadOffset) == (UFDTarr[fd].ptrfiletable->ptrinode->FileActualSize))
    {
        return -3;
    }

    if((UFDTarr[fd].ptrfiletable->ptrinode->FileType) != REGULAR)
    {
        return -4;
    }

    read_size = (UFDTarr[fd].ptrfiletable->ptrinode->FileActualSize) - (UFDTarr[fd].ptrfiletable->ReadOffset);

    if(read_size < isize)
    {
        strncpy(arr,(UFDTarr[fd].ptrfiletable->ptrinode->Buffer) + (UFDTarr[fd].ptrfiletable->ReadOffset),read_size);

        UFDTarr[fd].ptrfiletable->ReadOffset = UFDTarr[fd].ptrfiletable->ReadOffset + read_size;
    }
    else
    {
        strncpy(arr,(UFDTarr[fd].ptrfiletable->ptrinode->Buffer) + (UFDTarr[fd].ptrfiletable->ReadOffset),isize);

        UFDTarr[fd].ptrfiletable->ReadOffset = UFDTarr[fd].ptrfiletable->ReadOffset + isize;
    }

    return isize;
    
}

int OpenFile(char *name,int mode)
{
    int i=0;

    PINODE temp = NULL;

    if(name == NULL || mode <= 0)
    {
        return -1;
    }

    temp = Get_Inode(name);

    if(temp == NULL)
    {
        return -2;
    }

    if(temp ->Permission < mode)
    {
        return -3;
    }

    while(i < 50)
    {
        if(UFDTarr[i].ptrfiletable == NULL)
        {
            break;
        }
        i++;
    }

    UFDTarr[i].ptrfiletable = (PFILETABLE)malloc(sizeof(FILETABLE));

    if(UFDTarr[i].ptrfiletable == NULL)
    {
        return -1;
    }

    UFDTarr[i].ptrfiletable->Count = 1;
    UFDTarr[i].ptrfiletable->Mode = mode;

    if(mode == READ + WRITE)
    {
        UFDTarr[i].ptrfiletable->ReadOffset = 0;
        UFDTarr[i].ptrfiletable->WriteOffset = 0;
    }
    else if(mode == READ)
    {
        UFDTarr[i].ptrfiletable->ReadOffset = 0;
    }
    else if(mode == WRITE)
    {
        UFDTarr[i].ptrfiletable->WriteOffset = 0;
    }
    UFDTarr[i].ptrfiletable->ptrinode = temp;

    (UFDTarr[i].ptrfiletable->ptrinode->ReferenceCount)++;

    return i;
}

void CloseFileByName(int fd)
{
    UFDTarr[fd].ptrfiletable->ReadOffset = 0;
    UFDTarr[fd].ptrfiletable->WriteOffset = 0;
    (UFDTarr[fd].ptrfiletable->ptrinode->ReferenceCount)--;
}

int CloseFileByName(char *name)
{
    int fd=0;

    fd = GetFDFromName(name);

    if(fd == -1)
    {
        return -1;
    }

    UFDTarr[fd].ptrfiletable->ReadOffset = 0;
    UFDTarr[fd].ptrfiletable->WriteOffset = 0;
    (UFDTarr[fd].ptrfiletable->ptrinode->ReferenceCount)--;

    return 0;
}

void CloseAllFiles()
{
    int i=0;

    while(i < 50)
    {
        if(UFDTarr[i].ptrfiletable != NULL)
        {
            UFDTarr[i].ptrfiletable->ReadOffset = 0;
            UFDTarr[i].ptrfiletable->WriteOffset = 0;
            (UFDTarr[i].ptrfiletable->ptrinode->ReferenceCount)--;
            break;
        }
        i++;
    }
}

int LseekFile(int fd,int size,int from)
{
    if((fd < 0) || (from > 2))
    {
        return -1;
    }
    if(UFDTarr[fd].ptrfiletable == NULL)
    {
        return -1;
    }
    if((UFDTarr[fd].ptrfiletable->Mode == READ) || (UFDTarr[fd].ptrfiletable->Mode == READ + WRITE))
    {
        if(from == CURRENT)
        {
            if(((UFDTarr[fd].ptrfiletable->ReadOffset) + size) > UFDTarr[fd].ptrfiletable->ptrinode->FileActualSize)
            {
                return -1;
            }
            if(((UFDTarr[fd].ptrfiletable->ReadOffset) + size) < 0)
            {
                return -1;
            }

            (UFDTarr[fd].ptrfiletable->ReadOffset) = ((UFDTarr[fd].ptrfiletable->ReadOffset) + size);
        }

        else if(from == START)
        {
            if(size > (UFDTarr[fd].ptrfiletable->ptrinode->FileActualSize))
            {
                return -1;
            }

            if(size < 0)
            {
                return -1;
            }

            (UFDTarr[fd].ptrfiletable->ReadOffset) = size;
        }

        else if(from == END)
        {
            if((UFDTarr[fd].ptrfiletable->ptrinode->FileActualSize) + size >MAXFILESIZE)
            {
                return -1;
            }
            if(((UFDTarr[fd].ptrfiletable->ReadOffset) + size) < 0)
            {
                return -1;
            }

            (UFDTarr[fd].ptrfiletable->ReadOffset) = (UFDTarr[fd].ptrfiletable->ptrinode->FileActualSize) + size;
        }

        else if(UFDTarr[fd].ptrfiletable->Mode == WRITE)
        {
            if(from == CURRENT)
            {
                if(((UFDTarr[fd].ptrfiletable->WriteOffset) + size) > MAXFILESIZE)
                {
                    return -1;
                }
                if(((UFDTarr[fd].ptrfiletable->WriteOffset) + size) < 0)
                {
                    return -1;
                }
                if(((UFDTarr[fd].ptrfiletable->WriteOffset) + size) > MAXFILESIZE)
                {
                    return -1;
                }
                if(((UFDTarr[fd].ptrfiletable->WriteOffset) + size) > (UFDTarr[fd].ptrfiletable->ptrinode->FileActualSize))
                {
                    return -1;
                }

                (UFDTarr[fd].ptrfiletable->ptrinode->FileActualSize) = (UFDTarr[fd].ptrfiletable->WriteOffset) + size;

                (UFDTarr[fd].ptrfiletable->WriteOffset) = (UFDTarr[fd].ptrfiletable->WriteOffset) = size;
            }

            else if(from == START)
            {
                if(size > MAXFILESIZE)
                {
                    return -1;
                }
                
                if(size < 0)
                {
                    return -1;
                }
                
                if(size > (UFDTarr[fd].ptrfiletable->ptrinode->FileActualSize))
                {
                    (UFDTarr[fd].ptrfiletable->ptrinode->FileActualSize) = size;
                }

                (UFDTarr[fd].ptrfiletable->WriteOffset) = size;
            }

            else if(from == END)
            {
                if((UFDTarr[fd].ptrfiletable->ptrinode->FileActualSize) + size >MAXFILESIZE)
                {
                    return -1;
                }
                if((UFDTarr[fd].ptrfiletable->WriteOffset) + size < 0)
                {
                    return -1;
                }

                (UFDTarr[fd].ptrfiletable->WriteOffset) = (UFDTarr[fd].ptrfiletable->ptrinode->FileActualSize) + size;
                
            }
        }
    }
    
    

}

int fstat_file(int fd)
{
    PINODE temp = Head;

    int i = 0;

    if(fd < 0)
    {
        return -1;
    }
    if(UFDTarr[fd].ptrfiletable == NULL)
    {
        return -2;
    }

    temp = UFDTarr[fd].ptrfiletable->ptrinode;

    printf("\n------Statical Information about the file---------\n");
    printf("File name : %s\n",temp->FileName);
    printf("Inode Number : %d\n",temp->InodeNumber);
    printf("File Size : %d\n",temp->FileSize);
    printf("Actual File Size : %d\n",temp->FileActualSize);
    printf("Link count : %d\n",temp->LinkCount);
    printf("Reference Count : %d\n",temp->ReferenceCount);

    if(temp->Permission == 1)
    {
        printf("File permission : Read Only\n");
    }
    
    else if(temp->Permission == 2)
    {
        printf("File permission : Write Only\n");
    }
    else if(temp->Permission == 3)
    {
        printf("File permission : Read and Write\n");
    }

    printf("-------------------------------------------------------------\n");

    return 0;

}

int Stat_File(char * name)
{
    PINODE temp = Head;

    int i=0;

    if(name == NULL)
    {
        return -1;
    }

    while(temp != NULL)
    {
        if(strcmp(name,temp->FileName) == 0)
        {
            break;
        }
        temp = temp->next;
    }
    if(temp == NULL)
    {
        return -2;
    }

    printf("\n-------Statical information about the file------------------------\n");
    printf("File name : %s\n",temp->FileName);
    printf("Inode Number : %d\n",temp->InodeNumber);
    printf("File Size : %d\n",temp->FileSize);
    printf("Actual File Size : %d\n",temp->FileActualSize);
    printf("Link count : %d\n",temp->LinkCount);
    printf("Reference Count : %d\n",temp->ReferenceCount);

    if(temp->Permission == 1)
    {
        printf("File Permission : Read Only\n");
    }
    else if(temp->Permission == 2)
    {
        printf("File permission : Write Only\n");
    }
    else if(temp->Permission == 3)
    {
        printf("File permission : Read and Write\n");
    }

    printf("-------------------------------------------------------------\n");

    return 0;    
}

void man(char *name)
{
    if(name == NULL)
    {
        return;
    }

    if(strcmp(name,"create") == 0)
    {
        printf("Description : Used to create the new regular File\n");
        printf("Usages : creat File_name permission\n");
    }
    else if(strcmp(name,"read") == 0)
    {
        printf("Description : Used to read the data in the regular File\n");
        printf("Usages : read File_name number_of_bytes_to_read\n");
    }
    else if(strcmp(name,"write") == 0)
    {
        printf("Description : Used to write into the new regular File\n");
        printf("Usages : write File_name \n after this enter the data that we want to write\n");
    }
    else if(strcmp(name,"ls") == 0)
    {
        printf("Description : Used to list out all the regular File\n");
        printf("Usages : ls\n");
    }
    else if(strcmp(name,"rm") == 0)
    {
        printf("Description : Used to delete the regular File\n");
        printf("Usages : rm file name\n");
    }
    else if(strcmp(name,"stat") == 0)
    {
        printf("Description : Used to display the information of the regular File\n");
        printf("Usages : stat file name\n");
    }
    else if(strcmp(name,"fstat") == 0)
    {
        printf("Description : Used to display the information of the regular File\n");
        printf("Usages : fstat file name\n");
    }
    else if(strcmp(name,"truncate") == 0)
    {
        printf("Description : Used to Remove data from trhe file\n");
        printf("Usages : truncate file name\n");
    }
    else if(strcmp(name,"open") == 0)
    {
        printf("Description : Used to open the existing file\n");
        printf("Usages : open file name mode\n");
    }
    else if(strcmp(name,"close") == 0)
    {
        printf("Description : Used to close the opened existing file\n");
        printf("Usages : close file name\n");
    }
    else if(strcmp(name,"closeall") == 0)
    {
        printf("Description : Used to close all opened file\n");
        printf("Usages : closeall\n");
    }
    else if(strcmp(name,"lseek") == 0)
    {
        printf("Description : Used to change the offset\n");
        printf("Usages : lseek File_name change_in_offset start_point\n");
    }
    else
    {
        printf("ERROR : No manual entry found\n");
    }
    
}

int truncate_file(char *name)
{
    int fd = GetFDFromName(name);

    if(fd == -1)
    {
        return -1;
    }

    memset(UFDTarr[fd].ptrfiletable->ptrinode->Buffer,0,1024);

    UFDTarr[fd].ptrfiletable->ReadOffset = 0;
    UFDTarr[fd].ptrfiletable->WriteOffset = 0;
    UFDTarr[fd].ptrfiletable->ptrinode->FileActualSize = 0;
}

void DisplayHelp()
{
    printf("ls : To list out all the files\n");
    printf("cls : To clear the console\n");
    printf("open : To open the the files\n");
    printf("close : To close the files\n");
    printf("closeall : To close all the files\n");
    printf("read : To read the content from the files\n");
    printf("write : To write in the files\n");
    printf("exit : to terminate the system\n");
    printf("stat : to display the information using the name\n");
    printf("fstat : to display the information using the FD\n");
    printf("truncate : to remove the all the data from the file\n");
    printf("rm : to delte the file\n");   
}

int main()
{
    int count=0;
    char str[50];
    int ret=0;
    int fd=0;
    char command[4][80];
    char arr[1024];
    char *ptr = NULL;

    printf("CVFS\n");

    CreateDILB();
    InitialiseSuperBlock();

    while(1)
    {
        printf("Satyam VFS -> ");

        scanf(" %[^\n]s",str);

        count = sscanf(str, "%s %s %s %s",command[0],command[1],command[2],command[3]);
    
        if(count == 1)
        {
            if(strcmp(command[0],"help") == 0)
            {
                DisplayHelp();
                continue;
            }
            else if(strcmp(command[0],"cls") == 0)
            {
                system("cls");
                continue;
            }
            if(strcmp(command[0],"exit") == 0)
            {
                printf("Terminating the CVFS.....\n");
                break;
            }
            else if(strcmp(command[0],"ls") == 0)
            {
                ls_file();
            }
            else if(strcmp(command[0],"closeall") == 0)
            {
                CloseAllFiles();
                printf("All File close Succesfully\n");
                continue;
            }
            else
            {
                printf("ERROR : Command not Found!!\n");
                continue;
            }
        }

        else if(count == 2)
        {
            if(strcmp(command[0],"man") == 0)
            {
                man(command[1]);
            }
            else if(strcmp(command[0],"rm") == 0)
            {
                ret = rm_File(command[1]);

                if(ret == -1)
                {
                    printf("ERROR : There is no such a file\n");
                }
                continue;
            }
            else if(strcmp(command[0],"truncate") == 0)
            {
                ret = truncate_file(command[1]);

                if(ret == -1)
                {
                    printf("ERROR : Incorrect parameter\n");
                }
                
            }
            else if(strcmp(command[0],"fstat") == 0)
            {
                ret = fstat_file(atoi(command[1]));

                if(ret == -1)
                {
                    printf("ERROR : Incorrect parameter\n");
                }
                if(ret == -2)
                {
                    printf("ERROR : There is no such a file\n");
                }
                continue;
                
            }
            else if(strcmp(command[0],"stat") == 0)
            {
                ret = Stat_File(command[1]);

                if(ret == -1)
                {
                    printf("ERROR : Incorrect parameter\n");
                }
                if(ret == -2)
                {
                    printf("ERROR : There is no such a file\n");
                }
                continue;
                
            }
            else if(strcmp(command[0],"write") == 0)
            {
                fd = GetFDFromName(command[1]);

                if(fd == -1)
                {
                    printf("ERROR : Incorrect parametre\n");
                    continue;
                }
                printf("Enter the data\n");
                scanf(" %[^\n]s",arr);

                ret = strlen(arr);

                if(ret == 0)
                {
                    printf("ERROR : Incorrect parameter\n");
                    continue;
                }

                ret = WriteFile(fd,arr,ret);

                if(ret == -1)
                {
                    printf("ERROR : Permission denied\n");
                }
                if(ret == -2)
                {
                    printf("ERROR : There is no sufficient memory\n");
                }
                if(ret == -3)
                {
                    printf("ERROR : It is not a regular file\n");
                }
            }

            else if(strcmp(command[0],"close") == 0)
            {
                ret = CloseFileByName(command[1]);

                if(ret == -1)
                {
                    printf("ERROR : There is no such a file\n");
                }
                continue;
            }

            else
            {
                printf("ERROR : Command not found!!!\n");
                continue;
            }
            
        }

        else if(count == 3)
        {
            if(strcmp(command[0],"creat") == 0)
            {
                int ret = CreateFile(command[1],atoi(command[2]));
                
                if(ret >= 0)
                {
                    printf("File successfully created with FD  %d\n",ret);
                }
                if(ret == -1)
                {
                    printf("ERROR : Incorrect parameter\n");
                }
                if(ret == -2)
                {
                    printf("ERROR : There is no inode\n");
                }
                if(ret == -3)
                {
                    printf("ERROR : File already exit\n");
                }
                if(ret == -4)
                {
                    printf("ERROR : Memory allocation failure\n");
                }
                continue;
            }

            else if(strcmp(command[0],"open") == 0)
            {
                ret = OpenFile(command[1],atoi(command[2]));

                if(ret >= 0)
                {
                    printf("File suucessfully open with the FD %d\n",ret);
                }
                if(ret == -1)
                {
                    printf("ERROR : Incorrect parameter\n");
                }
                if(ret == -2)
                {
                    printf("ERROR : File not present\n");
                }
                if(ret == -3)
                {
                    printf("ERROR : Permission Denied\n");
                }
                continue;
            }

            else if(strcmp(command[0],"read") == 0)
            {
                fd = GetFDFromName(command[1]);

                if(fd == -1)
                {
                    printf("ERROR : Incorrect parametre\n");
                    continue;
                }

                ptr = ((char*)malloc(atoi(command[2]))+1);

                if(ptr == NULL)
                {
                    printf("ERROR : Memory allocation fails\n");
                    continue;
                }

                ret = ReadFile(fd,ptr,atoi(command[2]));
                
                if(ret == -1)
                {
                    printf("ERROR : File not existing\n");
                }

                if(ret == -2)
                {
                    printf("ERROR : permission denied\n");
                }
                if(ret == -3)
                {
                    printf("ERROR : Reached at the end of the file\n");
                }
                if(ret == -4)
                {
                    printf("ERROR : It is not a regular file\n");
                }
                if(ret > 0)
                {
                    write(2,ptr,ret);
                    printf("\n");
                }
                continue;

            }
            else
            {
                printf("\nERROR : Command not found\n");
                continue;
            }
            
        }

        else if(count == 4)
        {
            if(strcmp(command[0],"lseek") == 0)
            {
                fd = GetFDFromName(command[1]);

                if(fd == -1)
                {
                    printf("ERROR : Incorrect parameter\n");
                    continue;
                }

                ret = LseekFile(fd,atoi(command[2]),atoi(command[3]));

                if(ret == -1)
                {
                    printf("ERROR : Unable to perform lseek\n");
                }
            }
            else
            {
                printf("ERROR : Command not found !!!\n");
                continue;
            }
            
        }
        else
        {
            printf("ERROR : Comand Not found !!!\n");
            continue;
        }
        
    }
    return 0;
}