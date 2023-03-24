#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

int main()
{
       int fd_0;
       int fd_1;
       int fd_2;


       fd_0 = open("/dev/demo", O_RDWR);                                       //1
       write(fd_0, "", 1);                                                                     //2
       read(fd_0, "", 1);                                                                      //3
       write(fd_0, "", 1);     //4
       fd_1 = open("/dev/demo", O_RDWR);                                       //5
       ioctl(fd_1, 0, 0);                                                                            //6
       write(fd_1, "", 1);     //7
       write(fd_1, "", 1);     //8
       write(fd_0, "", 1);     //9
       read(fd_1, "", 1);                                              //10
       ioctl(fd_1, 0, 0);                                                                            //11
       read(fd_1, "", 1);                                              //12
       lseek(fd_0, 1, 1);                                                                              //13
       close(fd_0);                                                                            //14
       close(fd_1);                                                                            //15
       fd_2 = open("/dev/demo", O_RDWR);                                       //16
       read(fd_2, "", 1);                                              //17
       lseek(fd_2, 1, 1);                                                                              //18
       read(fd_2, "", 1);                                              //19
       write(fd_2, "", 1);     //20
       close(fd_2);                                                                            //21

}