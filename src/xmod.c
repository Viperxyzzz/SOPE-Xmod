#include <stdio.h>
#include <stdlib.h> 
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h> 
#include <signal.h>
#include<wait.h>




extern int errno;


//Sinais
int nfmod = 0;
int nftot = 0;
int child_total = 0;
pid_t child_pid[100];
char* signal_filepath;
char* signal_parent_set;

//---------


/* Function to Check no is in octal
or not */
bool isOctal(long n)
{
    while (n)
    {
        if ((n % 10) >= 8)
            return false;
        else
            n = n / 10;
    }
    return true;
}

mode_t getumask(){
    mode_t mask = umask( 0 );
    umask(mask);
    return mask;
}

int getChmod(const char *path){
    struct stat ret;

    if (stat(path, &ret) == -1) {
        return -1;
    }

    return (ret.st_mode & S_IRUSR)|(ret.st_mode & S_IWUSR)|(ret.st_mode & S_IXUSR)|/*owner*/
        (ret.st_mode & S_IRGRP)|(ret.st_mode & S_IWGRP)|(ret.st_mode & S_IXGRP)|/*group*/
        (ret.st_mode & S_IROTH)|(ret.st_mode & S_IWOTH)|(ret.st_mode & S_IXOTH);/*other*/
}

bool isDirectory(char* filename)
{
    struct stat path_stat;
    int status = stat(filename, &path_stat);
    if(status != 0)
    {
        perror("Error");
        return 0;
    }
    if(S_ISREG(path_stat.st_mode))
    {
        return 0;
    }
    else if(S_ISDIR(path_stat.st_mode)) return 1;
    return 0;
}


//Signals ---------

void sigcont_handler(int sig)
{
    for(int i = 0; i < child_total;i++)
        kill(child_pid[i],SIGCONT);
    
}


void wait_processes(){
   char input;
   fprintf(stdout, "Do you wish to continue y / n : ");
   do
   {
        scanf("%c",&input);
        if(input == 'y')
        {
            for(int i = 0; i < child_total; i++)
            {
                kill(child_pid[i],SIGCONT);
            }
            break;
        }
        else if(input == 'n')
        {
            for(int i = 0; i < child_total;i++)
            {
                kill(child_pid[i],SIGKILL);
            }
            exit(-1);
        }
   } while(1);
}

void print_signal(char* filepath)
{
    fprintf(stdout,"%d; %s; %d; %d\n",getpid(),filepath,nftot,nfmod);
}


void signal_wrapper()
{
    if(signal_parent_set != NULL) //se filho
    {
        print_signal(signal_filepath);
        struct sigaction new;
        sigset_t smask;

        if(sigemptyset(&smask) == -1)
            perror("sigsetfunctions");
        new.sa_handler = sigcont_handler;
        new.sa_mask = smask;
        new.sa_flags = 0;
        if(sigaction(SIGCONT,&new,NULL) == -1)
            perror("sigaction");
        pause();
    }
    else 
    {
        pid_t wpid;
        int val = 0;
        print_signal(signal_filepath);
        wait_processes();
        while((wpid = wait(&val)) > 0)
        {
            ;
        }
    }

}

void sig_handler(int signum)
{
    signal_wrapper();

}



//------------------------- rwx converter

void rwxmode(mode_t mode, char * buf) {
  const char chars[] = "rwxrwxrwx";
  for (size_t c = 0; c < 9; c++) {
    buf[c] = (mode & (1 << (8-c))) ? chars[c] : '-';
  }
  buf[9] = '\0';
}


//-------------------------

int change_perm(char* filepath, bool v, bool c, mode_t new_mode, mode_t old_mode )
{

    char old[10], new[10];   
    rwxmode(old_mode, old);
    rwxmode(new_mode, new);

    nftot++;
    //See if file exists and can acesss it
    int returnval = access (filepath, F_OK);
    if (returnval != 0)
    {
        if (errno == ENOENT){
            perror("Error");
            return 1;
        }else if (errno == EACCES){
            perror("Error");
            return 1;
        }
    }else{
        //Change permissions
        if(chmod(filepath, new_mode) != 0)
        {
            perror("Error");
            return 1;
        }else{
            if(v)
            {
                if(old_mode == new_mode)
                {                
                    printf("mode of '%s'  retained as %o (%s) \n",  filepath, old_mode, old);
                }else{
                    nfmod++;
                    printf("mode of '%s' changed from %o (%s) to %o (%s) \n",  filepath, old_mode, old, new_mode, new);
                }
            }else if(c){
                if(old_mode != new_mode)
                {
                    nfmod++;
                    printf("mode of '%s' changed from %o (%s) to %o (%s) \n",  filepath, old_mode, old, new_mode, new);
                }
            }
            return 0;
        }
    }
    return 1;
}



int main(int argc, char** argv)
{   

    //Informaçao para o log
    clock_t time_begin = clock();
    pid_t process_id = getpid();
    


    //Criar e abrir log 
    bool log = true;
    char* log_val = getenv("LOG_FILENAME"); 
    char* parent_set = getenv("XMOD");
    FILE *log_file;
    if(log_val == NULL)
    {
        log = false;
    }
    if(parent_set == NULL)
    {
        putenv("XMOD=SET");
    }
    if(log){ 
        if(parent_set == NULL) //é o pai de todos
        {
            putenv("XMOD=SET");
            log_file = fopen(log_val, "w+");
            if (log_file == NULL) 
            {perror("Error"); return 1;}

        }else{ //filhos 
            log_file = fopen(log_val, "a");
            if (log_file == NULL) 
            {perror("Error"); return 1;}
        }
    }



    if(log)
    { 
        setbuf(log_file, NULL);
        clock_t end = clock();
        double instant = (double)(end- time_begin) / CLOCKS_PER_SEC;
        fprintf(log_file, "%f ; %d ; %s ",instant, process_id, "PROC_CREAT");
        for(int j = 0 ; j < argc ; j++)
        {
            fprintf(log_file, "; %s ", argv[j]);
        }
        fputs("\n", log_file);
    }

    bool v, c, r = false;

    int i = 1;
    if (argc <  4)
    {
        fprintf(stderr, "Error: too little arguments \n");
        return -1;
    }







    //      OPTIONS     //
    while(strncmp(argv[i], "-", 1) == 0)
    {
        if(strcmp(argv[i], "-v") == 0)
        {
            v = true;
        }else if(strcmp(argv[i], "-c") == 0){
            c = true;
        }else if(strcmp(argv[i], "-R") == 0){
            r = true;
        }else {
            fprintf(stderr, "Error : options can only be -c -R or -v \n");
        }
        i++;
        if(i == argc)
        {
            fprintf(stderr, "Error : Needs to specify either MODE or OCTAL-MODE\n");   
            return -1;
        }
    }




    //      MODE OR OCTAL        //
    char* mode = argv[i];

    //      FILE        //
    i++;
    char* filepath = argv[i];

    //Signals--------------------


    signal_filepath = filepath;
    signal_parent_set = parent_set;
    struct sigaction new_sigint;
    sigset_t smask;

    if(sigemptyset(&smask) == -1)
        perror("sigsetfunctions");

    new_sigint.sa_handler = sig_handler;
    new_sigint.sa_mask = smask;
    new_sigint.sa_flags = 0;

    if(sigaction(SIGINT,&new_sigint,NULL) == -1)
        perror("sigaction");

    mode_t old_mode = getChmod(filepath);
    mode_t new_mode = old_mode;


    //--------------------------



    //      MODE OR OCTAL MODE        //
    if(strncmp(mode, "u", 1) == 0) //Mode user
    {
        if(mode[1] == '+'){ //Add in User
            for(int j = 2; j < strlen(mode); j++)
            {
                if(mode[j] == 'r')
                {
                    new_mode |= S_IRUSR;
                }
                if(mode[j] == 'w')
                {
                    new_mode |= S_IWUSR;                    
                }
                if(mode[j] == 'x')
                {
                    new_mode |= S_IXUSR  ;
                }
                if(mode[j] != 'r' && mode[j] != 'w' && mode[j] != 'x') {
                    printf("Asking for wrong permissions! \n");
                }
            }
        }
        else if(mode[1] == '-') //Remove in User
        {
            for(int j = 2; j < strlen(mode); j++)
            {
                if(mode[j] == 'r')
                {
                    new_mode &= ~S_IRUSR;
                }
                if(mode[j] == 'w')
                {
                    new_mode &= ~S_IWUSR;                    
                }
                if(mode[j] == 'x')
                {
                    new_mode &= ~S_IXUSR;
                }
                if(mode[j] != 'r' && mode[j] != 'w' && mode[j] != 'x')  {
                    printf("Asking for wrong permissions! \n");
                }
            }
        }
        else if(mode[1] == '=') //Sustitute in User
        {
            //Clean
            new_mode &= ~S_IRUSR;
            new_mode &= ~S_IWUSR;  
            new_mode &= ~S_IXUSR;
            //Add wanted
            for(int j = 2; j < strlen(mode); j++)
            {
                if(mode[j] == 'r')
                {
                    new_mode |= S_IRUSR;
                }
                if(mode[j] == 'w')
                {
                    new_mode |= S_IWUSR;                    
                }
                if(mode[j] == 'x')
                {
                    new_mode |= S_IXUSR  ;
                }
                if(mode[j] != 'r' && mode[j] != 'w' && mode[j] != 'x')  {
                    printf("Asking for wrong permissions! \n");
                }
            }
        }else{printf("Asking for wrong permissions\n");}
    }else if(strncmp(mode, "g", 1) == 0){ //Mode group
        if(mode[1] == '+'){ //Add in group
            for(int j = 2; j < strlen(mode); j++)
            {
                if(mode[j] == 'r')
                {
                    new_mode |= S_IRGRP;
                }
                if(mode[j] == 'w')
                {
                    new_mode |= S_IWGRP;                    
                }
                if(mode[j] == 'x')
                {
                    new_mode |= S_IXGRP;
                }
                if(mode[j] != 'r' && mode[j] != 'w' && mode[j] != 'x')  {
                    printf("Asking for wrong permissions! \n");
                }
            }
        }else if(mode[1] == '-'){ //Remove in group
            for(int j = 2; j < strlen(mode); j++)
            {
                if(mode[j] == 'r')
                {
                    new_mode &= ~S_IRGRP;
                }
                if(mode[j] == 'w')
                {
                    new_mode &= ~S_IWGRP;                    
                }
                if(mode[j] == 'x')
                {
                    new_mode &= ~S_IXGRP;
                }
                if(mode[j] != 'r' && mode[j] != 'w' && mode[j] != 'x')  {
                    printf("Asking for wrong permissions! \n");
                }
            }
        }else if(mode[1] == '='){ //Sustitute in group
            //Clean
            new_mode &= ~S_IRGRP;
            new_mode &= ~S_IWGRP;  
            new_mode &= ~S_IXGRP;
            //Add wanted
            for(int j = 2; j < strlen(mode); j++)
            {
                if(mode[j] == 'r')
                {
                    new_mode |= S_IRGRP;
                }
                if(mode[j] == 'w')
                {
                    new_mode |= S_IWGRP;                    
                }
                if(mode[j] == 'x')
                {
                    new_mode |= S_IXGRP  ;
                }
                if(mode[j] != 'r' && mode[j] != 'w' && mode[j] != 'x')  {
                    printf("Asking for wrong permissions! \n");
                }
            }
        }else{printf("Asking for wrong permissions\n");}
    }else if(strncmp(mode, "o", 1) == 0){ //Mode other
        if(mode[1] == '+'){ //Add in other
            for(int j = 2; j < strlen(mode); j++)
            {
                if(mode[j] == 'r')
                {
                    new_mode |= S_IROTH  ;
                }
                if(mode[j] == 'w')
                {
                    new_mode |= S_IWOTH  ;                    
                }
                if(mode[j] == 'x')
                {
                    new_mode |= S_IXOTH  ;
                }
                if(mode[j] != 'r' && mode[j] != 'w' && mode[j] != 'x')  {
                    printf("Asking for wrong permissions! \n");
                }
            }
        }else if(mode[1] == '-'){ //Remove in other
            for(int j = 2; j < strlen(mode); j++)
            {
                if(mode[j] == 'r')
                {
                    new_mode &= ~S_IROTH;
                }
                if(mode[j] == 'w')
                {
                    new_mode &= ~S_IWOTH;                    
                }
                if(mode[j] == 'x')
                {
                    new_mode &= ~S_IXOTH;
                }
                if(mode[j] != 'r' && mode[j] != 'w' && mode[j] != 'x')  {
                    printf("Asking for wrong permissions! \n");
                }
            }
        }else if(mode[1] == '='){ //Sustitute in other
            //Clean
            new_mode &= ~S_IROTH;
            new_mode &= ~S_IWOTH;  
            new_mode &= ~S_IXOTH;
            //Add wanted
            for(int j = 2; j < strlen(mode); j++)
            {
                if(mode[j] == 'r')
                {
                    new_mode |= S_IROTH;
                }
                if(mode[j] == 'w')
                {
                    new_mode |= S_IWOTH;                    
                }
                if(mode[j] == 'x')
                {
                    new_mode |= S_IXOTH  ;
                }
                if(mode[j] != 'r' && mode[j] != 'w' && mode[j] != 'x')  {
                    printf("Asking for wrong permissions! \n");
                }
            }
        }else { printf("Asking for wrong permissions\n"); }
    }else if(strncmp(mode, "a", 1) == 0){ //Mode all
        if(mode[1] == '+'){ //Add to all
            for(int j = 2; j < strlen(mode); j++)
            {
                if(mode[j] == 'r')
                {
                    new_mode = new_mode | S_IRUSR  | S_IRGRP  | S_IROTH;
                }
                if(mode[j] == 'w')
                {
                    new_mode = new_mode | S_IWUSR  | S_IWGRP  |  S_IWOTH;                    
                }
                if(mode[j] == 'x')
                {
                    new_mode = new_mode | S_IXUSR  | S_IXGRP  | S_IXOTH;
                }
                if(mode[j] != 'r' && mode[j] != 'w' && mode[j] != 'x')  {
                    printf("Asking for wrong permissions! \n");
                }
            }
        }else if(mode[1] == '-'){ //Remove to all
            for(int j = 2; j < strlen(mode); j++){
                if(mode[j] == 'r')
                {
                    new_mode &= ~S_IRUSR;
                    new_mode &= ~S_IRGRP;
                    new_mode &= ~S_IROTH;
                }
                if(mode[j] == 'w')
                {
                    new_mode &= ~S_IWUSR;
                    new_mode &= ~S_IWGRP;                    
                    new_mode &= ~S_IWOTH;                                        
                }
                if(mode[j] == 'x')
                {
                    new_mode &= ~S_IXUSR;
                    new_mode &= ~S_IXGRP;
                    new_mode &= ~S_IXOTH;

                }
                if(mode[j] != 'r' && mode[j] != 'w' && mode[j] != 'x')  {
                    printf("Asking for wrong permissions! \n");
                }
            }
        }else if(mode[1] == '='){ //Sustitute to all
            //Clean
            //Clean execute
            new_mode &= ~S_IXUSR;
            new_mode &= ~S_IXGRP;
            new_mode &= ~S_IXOTH;
            //Clean write
            new_mode &= ~S_IWUSR;
            new_mode &= ~S_IWGRP;                    
            new_mode &= ~S_IWOTH;
            //Clean read
            new_mode &= ~S_IRUSR;
            new_mode &= ~S_IRGRP;
            new_mode &= ~S_IROTH;     
            //Add wanted
            for(int j = 2; j < strlen(mode); j++)
            {
                if(mode[j] == 'r')
                {
                    new_mode = new_mode | S_IRUSR  | S_IRGRP  | S_IROTH;
                }
                if(mode[j] == 'w')
                {
                    new_mode = new_mode | S_IWUSR  | S_IWGRP  |  S_IWOTH;                    
                }
                if(mode[j] == 'x')
                {
                    new_mode = new_mode | S_IXUSR  | S_IXGRP  | S_IXOTH;
                }
                if(mode[j] != 'r' && mode[j] != 'w' && mode[j] != 'x')  {
                    printf("Asking for wrong permissions! \n");
                }
            }
        }else{printf("Asking for wrong permissions\n");}


    }else if(strncmp(mode, "-", 1) == 0 || strncmp(mode, "+", 1) == 0 || strncmp(mode, "=", 1) == 0){ //Mode with Omission
        printf("Didnt specify the user \n");
    }else if(isOctal(atoi(mode)) && strlen(mode) == 4){ //Octal Mode
        new_mode = strtol(mode, NULL, 8);
    }
    
    if(r)
    {
        struct dirent *de;  // Pointer for directory entry 
        DIR* directory = opendir(filepath);
        if (directory == NULL)// opendir returns NULL if couldn't open directory
        {
            change_perm(filepath, v, c, new_mode, old_mode);
            if(log)
            {
                setbuf(log_file, NULL);
                clock_t end = clock();
                double instant = (double)(end- time_begin) / CLOCKS_PER_SEC;
                fprintf(log_file, "%f ; %d ; %s ; %s : %o : %o \n", instant, process_id, "FILE_MODF", filepath, old_mode, new_mode); 
            }
        }else {
            change_perm(filepath, v, c, new_mode, old_mode);
            if(log)
            {        
                setbuf(log_file, NULL);
                clock_t end = clock();
                double instant = (double)(end- time_begin) / CLOCKS_PER_SEC;
                fprintf(log_file, "%f ; %d ; %s ; %s : %o : %o \n", instant, process_id, "FILE_MODF", filepath, old_mode, new_mode); 
            }
            while ((de = readdir(directory)) != NULL){
                if(strcmp(de->d_name, ".") && strcmp(de->d_name, ".."))
                {
                    char* new_filepath = malloc(sizeof(filepath) + sizeof("/") + sizeof(de->d_name));
                    snprintf(new_filepath, sizeof(filepath) + sizeof("/") + sizeof(de->d_name) , "%s/%s", filepath, de->d_name);
                    if(isDirectory(new_filepath)){
                        int val = 0;
                        pid_t child = fork();
                        pid_t wpid;
                        if(child == 0) //Its the child
                        {
                            argv[i] = new_filepath;
                            if(log)
                                fclose(log_file);
                            execvp("./xmod",argv);
                            return 0;
                        }else{
                            child_pid[child_total] = child;
                            child_total++;
                            while((wpid = wait(&val)) > 0)
                            {
                                ;
                            }
                        }
                    }else{
                        change_perm(new_filepath, v, c, new_mode, old_mode);
                        if(log)
                        {
                            setbuf(log_file, NULL);
                            clock_t end = clock();
                            double instant = (double)(end- time_begin) / CLOCKS_PER_SEC;
                            fprintf(log_file, "%f ; %d ; %s ; %s : %o : %o \n", instant, process_id, "FILE_MODF", filepath, old_mode, new_mode); 

                        }
                    }
                }
            }
            closedir(directory);
        }
    }else{
        change_perm(filepath, v, c, new_mode, old_mode);
        if(log)
        {
            setbuf(log_file, NULL);
            clock_t end = clock();
            double instant = (double)(end- time_begin) / CLOCKS_PER_SEC;
            fprintf(log_file, "%f ; %d ; %s ; %s : %o : %o \n", instant, process_id, "FILE_MODF", filepath, old_mode, new_mode); 
        }
    }
    if(log)
    {
        setbuf(log_file, NULL);
        clock_t end = clock();
        double instant = (double)(end- time_begin) / CLOCKS_PER_SEC;
        fprintf(log_file, "%f ; %d ; %s ; %s \n", instant, process_id, "PROC_EXIT", "0"); 
    }



    if(log)
        fclose(log_file);
    
    return 0;

}