#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> //系统类型
#include <sys/wait.h> //waitpid
#include <fcntl.h> //dup2


struct {
     char* cmd;//
     char* prog;//程序名
     char* arg[10];//参数,最多10个
     char infile[32];//最长32
     char outfile[32];
}info[10];

char input[512];//用户输入

int myfd[9][2];//10个程序，9个管道


static void split1();//第一次分割，检测是否有"|"
static void split2(int n);//第二次分割，检测是否有“<”或“>”以及" "
static void myrun();


int main(int argc, char *argv[])
{
     int i;
mystart:
     for (i=0; i<=9; i++)
          bzero(&info[i],sizeof(info[i]));
     
     printf("myshell:");
     gets(input);

     split1();

     myrun();
     
     goto mystart;
     return 0;
}

void split1()
{
     char* str;

     int i;
     for (i=0,str=input; ; i++){
          info[i].cmd=strtok(str,"|");
		  str=NULL;
          if (info[i].cmd==NULL)
               break;
          split2(i);//若redirect是调用非线程安全的strtok的话，会覆盖mypipe的数据
     }

}

void split2(int n)
{
     /*
     //这个算法处理不了cat<file这样的，应该先检测<>
     int i;
     char* pretoken;
     char* token;
     char* saveptr;
     
     info[n].prog=strtok_r(info[n].cmd," ",&saveptr);
     
     for (i=0; ; i++){
          token=strtok_r(NULL," ",&saveptr);
      if (token==NULL)
           break;
      if (strncmp("<",token,1)==0){
           if (strlen(token)>1)
                info[n].infile=token+1;                
      }
      else if (strncmp(">",token,1)==0){
           if (strlen(token)>1)
                info[n].outfile=token+1;
      }
      else if (strcmp("<",pretoken)==0){
           info[n].infile=token;
      }
      else if (strcmp(">",pretoken)==0){
           info[n].outfile=token;
      }
      else {
           info[n].arg[i]=token;
      }
      pretoken=token;
     }
     */
     
     char *saveptr,*pos,*tmp1,*tmp2,*tmp3;
     int i;
     
     if ( (tmp1=strstr(info[n].cmd,"<")) != NULL){
          
          pos=tmp1;
          tmp1++;
          
          if (strncmp(tmp1," ",1) == 0)
               tmp1++;
          tmp2=strstr(tmp1," ");//<file1 >file2的情况
          tmp3=strstr(tmp1,">");//<file1>file2的情况

          if (tmp2==NULL && tmp3==NULL){//这么多条件同时只能有一个成立，所以是else if
               memcpy(info[n].infile,tmp1,strlen(tmp1));
               memset(pos,' ',strlen(pos));
          }
          else if (tmp2 == NULL){
               memcpy(info[n].infile,tmp1,tmp3-tmp1);
               memset(pos,' ',tmp3-pos);
          }
          else if (tmp3 == NULL){
               memcpy(info[n].infile,tmp1,tmp2-tmp1);
               memset(pos,' ',tmp2-pos);
          }
          else if (tmp3< tmp2){
               memcpy(info[n].infile,tmp1,tmp3-tmp1);
               memset(pos,' ',tmp3-pos);
          }
          else {
               memcpy(info[n].infile,tmp1,tmp2-tmp1);
               memset(pos,' ',tmp2-pos);
          }
     }
     if ( (tmp1=strstr(info[n].cmd,">")) != NULL){
          
          pos=tmp1;
          tmp1++;
          
          if (strncmp(tmp1," ",1) == 0)
               tmp1++;
          tmp2=strstr(tmp1," ");
          tmp3=strstr(tmp1,"<");

          if (tmp2==NULL && tmp3==NULL){
               memcpy(info[n].outfile,tmp1,strlen(tmp1));
               memset(pos,' ',strlen(pos));
          }
          else if (tmp2 == NULL){
               memcpy(info[n].outfile,tmp1,tmp3-tmp1);
               memset(pos,' ',tmp3-pos);
          }
          else if (tmp3 == NULL){
               memcpy(info[n].outfile,tmp1,tmp2-tmp1);
               memset(pos,' ',tmp2-pos);
          }
          else if (tmp3< tmp2){
               memcpy(info[n].outfile,tmp1,tmp3-tmp1);
               memset(pos,' ',tmp3-pos);
          }
          else {
               memcpy(info[n].outfile,tmp1,tmp2-tmp1);
               memset(pos,' ',tmp2-pos);
          }
               
          // info[n].outfile = (char*)malloc(tmp2-tmp1+1);
          /*bzero(info[n].outfile,32);
          tmp2 ? memcpy(info[n].outfile,tmp1,tmp2-tmp1) : memcpy(info[n].outfile,tmp1,strlen(tmp1));
          tmp2 ? memset(pos,' ',tmp2-pos) : memset(pos,' ',strlen(pos));//把<file变成一堆空格*/

     }

     info[n].arg[0]=info[n].prog=strtok_r(info[n].cmd," ",&saveptr);//第一个是程序名
     for (i=1; ;i++){
          info[n].arg[i]=strtok_r(NULL," ",&saveptr);
          if (info[n].arg[i] == NULL)
               break;
     }
}

void myrun()
{
     int i, status, fd;
     pid_t pid;
     
     for (i=0; info[i].prog!=NULL; i++){

          if (info[i+1].prog != NULL){//后面有程序，则创建管道myfd[i][]
               if (pipe(&myfd[i]) < 0) //也可以是pipe(myfd[i])，数组做优质变成指针
                    perror("pipe");
               
          }
          if ( (pid=fork()) < 0)
               perror("fork");
          else if (pid == 0){

               if (myfd[i][0] != 0){//本进程和后一进程之间的管道读端
                    close(myfd[i][0]);//关闭读端
                    dup2(myfd[i][1],STDOUT_FILENO);
               }
               if (i > 0)
                    if (myfd[i-1][0] != 0){//本进程与前一进程之间的管道读端
                         close(myfd[i-1][1]);//关闭写端
                         dup2(myfd[i-1][0],STDIN_FILENO);
                    }
               
               if (strlen(info[i].infile) != 0){ //不能是 if (info[i].infile != NULL)，因为infile不是指针
                    if ( (fd = open(info[i].infile,O_RDONLY)) < 0){
                         perror("open");
                         exit(-1);
                    }
                    dup2(fd,STDIN_FILENO);
                    close(fd);
               }

               if (strlen(info[i].outfile) != 0){
                    if ( (fd = open(info[i].outfile, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR)) < 0){
                         perror("open");
                         exit(-1);
                    }
                    dup2(fd,STDOUT_FILENO);
                    close(fd);
               }
                    
               execvp(info[i].prog, info[i].arg);
               perror("exec");
          }
          else
               //wait(&status);//不可以在这等待子程序终止，应该在for结束之后
                if (myfd[i][0] != 0){//本进程和后一进程之间的管道读端
                     //close(myfd[i][0]);//如果父进程关闭读端，读端引用变为0,因为第二个子进程还没产生,这样的话导致往写端write的进程终止
                     close(myfd[i][1]);
               }
               
     }
     
     //waitpid(pid, &status, 0);这个会阻塞
     wait(&status);
}
