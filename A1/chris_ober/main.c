#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <fcntl.h>


void print_prompt(void);
void process_input(char* input);
void run_exit(void);
void run_cd(char* argument);
void run_exec(char* input, char input_words[256][256]);
void run_exec_file(char input_words[256][256]);
int check_non_path_builtin(char input_words[256][256], int redirect_int, int input_file, int out_put_file);
void home_dir_sub(char* input);
void replace_char_with_string(char *str, char c, char *replace);
void process_conmmanmd(char input_words[256][256], int redirect_int, int input_file, int out_put_file);
int check_for_redirect(char input_words[256][256]);




int main()
{
    //Define variable to hold input
    char input[256];

    //Print prompt ("cwd$ ")
    print_prompt();

    //Get input and contine as long as ctrl^d isnt hit
    while(fgets(input, sizeof(input), stdin) != NULL)
    {
        //check for home directory substitution
        home_dir_sub(input);
        //Process the input and look to see if anything can be done with command
        process_input(input);
        //print the prompt again
        print_prompt();
        memset(input, '\0', sizeof(input));
    }

    printf("\n");
}

//Function to prinpt prompt (Current working directory followed by $ and space)
void print_prompt(void)
{
    //cwd holds current working directory as string
    char cwd[256];

    //If returned NULL its an error let user know, else print prompt
    if(getcwd(cwd, sizeof(cwd)) == NULL)
    {
        perror("getcwd error");
    }
    else
    {
        printf("%s$ ", cwd);
    }
}

//Function that decides what to do based on given user input
void process_input(char* input)
{
    //Create input_words to hold each word of input
    char input_words[256][256];
    char args[256][256];
    char redir[256][256];

    //Clear input_words
    memset(input_words, '\0', sizeof(input_words));
    memset(args, '\0', sizeof(args));
    memset(redir, '\0', sizeof(redir));

    //j tracks letters in words
    int j = 0;
    //k tracks words
    int k = 0;

    int inFile = 0;
    int outFile = 0;

    //Create loop over letters and words to write words into input_words
    for (int i = 0; i < strlen(input); i++)
    {
        //If space move to next word increasing j by 1
        if(isspace(input[i]))
        {
            j++;
            k = 0;
        }
        //If no space keep writing current word
        else
        {
            input_words[j][k] = input[i];
            k++;
        }
    }

    //Check if there is a redirect in command
    int red_check = check_for_redirect(input_words);

    //If there is a redirect
    if(red_check >= 0)
    {
        //Remove command and argument from input
        for (int i = 0; i < red_check; i++)
        {
            for( j = 0; j < strlen(input_words[i]); j++)
            {
                args[i][j] = input_words[i][j];
            }
        }

        //set counter to help remove file redirect from input
        int counter = 0;

        //Cycle through rest of input words to retrieve rest of input
        while(strcmp("", input_words[red_check]) != '\0')
        {
            for(int i = 0; i < strlen(input_words[red_check]); i++)
            {
                redir[counter][i] = input_words[red_check][i];
            }
            red_check++;
            counter++;
        }

        //Counters to help parse
        int red_counter = 0;
        int in_count = 0;
        int out_count = 0;

        //Check for just < or >
        if(strcmp("", redir[red_counter]) != 0 && strcmp("", redir[red_counter+1]) == '\0')
        {
            printf("no file to redirect.\n");
        }

        //cycle through each (<, FILENAME) pair
        while(strcmp("", redir[red_counter + 1]) != 0)
        {
            //Check for input
            if(redir[red_counter][0] == '<')
            {   
                //Open input file
                inFile = open(redir[red_counter + 1], O_RDONLY);
                if(inFile < 0)
                {
                    printf("File %s couldnt open: \n", redir[red_counter +1]);
                    break;
                }
                in_count++;

            }
            //Check for output
            else if(redir[red_counter][0] == '>')
            {
                //Open output file
                outFile = open(redir[red_counter + 1], O_WRONLY | O_CREAT, 0644);
                if(outFile < 0)
                {
                    printf("File %s couldnt open: \n", redir[red_counter +1]);
                    break;
                }
                out_count++;
            }
            else
            {

            }
            red_counter++;

        }

        //Check for which redirect was found
        //Both are found 
        if(in_count > 0 && out_count > 0)
        {
            process_conmmanmd(args, 3, inFile, outFile);
        }
        //Output is found
        else if(out_count > 0)
        {
           process_conmmanmd(args, 1, 0, outFile);
        }
        //Input is found
        else if(in_count > 0)
        {
            process_conmmanmd(args, 2, inFile, 0);
        }
    }
    //If no redirect
    else
    {
        process_conmmanmd(input_words, 0, 0, 0);
    }
    

}

//Function to exit
void run_exit(void)
{
    exit(0);
}

//Function to change directpry
void run_cd(char* argument)
{

    //If fail let user know
    if(chdir(argument) <  0)
    {
        perror("Could not change to dirrectory");
    }
}
//Function that formats argument list and runs exec
void run_exec(char* input, char input_words[256][256])
{
    //Create counter to help create argument list
    int counter = 0;
    //Create argument list
    char *args[256];

    //Loop while word starting after command are not empty
    while(strcmp("", input_words[counter+1]) != 0)
    {
        //start tracking words after the command
        args[counter] = input_words[counter+1];
        counter++;
       }
       //Put trailling null into argument list
       args[counter] = NULL;

       //run command and error check
       if(execv(args[0], args) < 0)
       {
           perror("Error with execv");
       }

      //Reset args
       memset(args, '\0', sizeof(args));
}

//Function formats argument list and runs exec (based on . or / command) and uses child process
void run_exec_file(char input_words[256][256])
{
    //set up for child process
    pid_t pid;
    int status;

    pid = fork();


    if (pid == 0)
    {
        //Child

        //Create counter to help create argument list
        int counter = 0;

        //Create argument list
        char *args[256];

        //Loop while input words is not empty
        while(strcmp("", input_words[counter]) != 0)
        {
            //start tracking words
            args[counter] = input_words[counter];
            counter++;
        }
        //run command and error check
        if(execv(args[0], args) < 0)
        {
            perror("Error with execv");
        }

        //reset args
        memset(args, '\0', sizeof(args));

        //exit child
        exit(0);
    }
    else
    {

        //Parrent

        //Wait for child
        waitpid(pid, &status, 0);
    }
}

//Check to see if command that failed is somewhere else in the enviornment
int check_non_path_builtin(char input_words[256][256], int redirect_int, int input_file, int out_put_file)
{
    //Retrieve enviornment, and set up other variables to help iterate through the enviornemnt
    char* env = getenv("PATH");
    char *env_temp = strdup(env);
    char *ptr;
    char *temp;

    int fdIn;
    int fdOut;

    struct stat statbuf;
    //Go through each peice of the enviornment and test to see if each is what we are looking for
    for(ptr = strtok_r(env_temp, ":", &temp); ptr != NULL; ptr = strtok_r(NULL, ":", &temp))
    {
        char new_file[256];
        snprintf(new_file, sizeof(new_file), "%s/%s", ptr, input_words[0]);

        struct stat statbuf;
        //If we find it create child process and run a similar execuation as run_exec
        if(stat(new_file, &statbuf) == 0)
        {
            //used for create chld process
            pid_t pid;
            int status;
            pid = fork();
            if (pid == 0)
            {
                    //Handle redirect
                    //No redirect
                    if(redirect_int == 0)
                    {
                    }
                    //Handle outputfile as new stdout
                    else if(redirect_int == 1)
                    {
                        dup2(out_put_file, STDOUT_FILENO);
                    }
                    //Handle inputfile as new stdin
                    else if(redirect_int == 2)
                    {
                        dup2(input_file, STDIN_FILENO);
                    }
                    //Handle inputfile and outputfile as new stdin and stdout
                    else if(redirect_int == 3)
                    {
                        dup2(out_put_file, STDOUT_FILENO);
                        dup2(input_file, STDIN_FILENO);
                    }
                    //Create counter to help create argument list
                    int counter = 1;
                    //Create argument list
                    char *args[256];

                    args[0] = new_file;


                    //Loop while word starting after command are not empty
                    while(strcmp("\0", input_words[counter]) != 0)
                    {
                    //start tracking words after the command
                    args[counter] = input_words[counter];
                    counter++;
                       }
                       //Put trailling null into argument list
                       args[counter] = NULL;

                       //run command and error check
                       if(execv(args[0], args) < 0)
                       {
                           perror("Error with execv");
                       }

                      //Reset args
                       memset(args, '\0', sizeof(args));

                       exit(0);
            }
            else
            {
                //Wait for child to finish
                //Close all processes
                if(redirect_int == 0)
                {
                }
                //Close output
                else if(redirect_int == 1)
                {
                    close(out_put_file);
                }
                //close input
                else if(redirect_int == 2)
                {
                    close(input_file);
                }
                //close both
                else if(redirect_int == 3)
                {
                    close(input_file);
                    close(out_put_file);
                }
                waitpid(pid, &status, 0);
                return 1;
            }
            return 0;
        }
    }
    return 0;
}

//Function checks to see if it can subtitute any ~ in command or argument
void home_dir_sub(char* input)
{
    //Create variable to hold a username incase we find one
    char username[256];
    
    //Loop through evert letter of input looking for ~
    for(int i = 0; i < strlen(input); i++)
    {
        //If we find ~
        if(input[i] == '~')
        {
            //Create variables to keep track of
            //start: What index we find ~
            //j: counter used to find username
            //k: used to play letters in username
            int start = i;
            int j = i;
            int k = 0;
            //Place letters in username
            while(input[j+1] != '\n' && input[j+1] != '/')
            {
                username[k] = input[j+1];
                k++;
                j++;
            }
            //Add '\0' to end of username
            username[k] = '\0';
            //if we did not find username
            if(strcmp("", username) == 0)
            {
                //make username the home eviornment name
                char* home_env = getenv("HOME");
                replace_char_with_string(input, '~', home_env);
            }
            //If we did find username
            else
            {
                struct passwd *p;
                //find what directory that username will give us and replace
                if ((p = getpwnam(username)) == NULL)
                {
                    perror("getpwnam() error");
                }
                else
                {
                    strncpy(input + start, p->pw_dir, strlen(username));
                }
            }
        }
    }
    
    //reset username
    memset(username, '\0', sizeof(username));
}

//Function to help replace a char with a string we provide
void replace_char_with_string(char *str, char c, char *replace) {
    char *p = strchr(str, c);
    if (p) {
        char temp[strlen(str) + strlen(replace) - 1];
        strncpy(temp, str, p - str);
        temp[p - str] = '\0';
        strcat(temp, replace);
        strcat(temp, p + 1);
        strcpy(str, temp);
    }
}

void process_conmmanmd(char input_words[256][256], int redirect_int, int input_file, int out_put_file)
{
    //Check if user entered exit
    if(strcmp("exit", input_words[0]) == 0)
    {
        //run exit command
        run_exit();
    }
    //Check if user entered cd with exactly 1 argument
    else if(strcmp("cd", input_words[0]) == 0 && (strcmp("", input_words[1]) != 0) && (strcmp("", input_words[2]) == 0))
    {
        //run cd command
        run_cd(input_words[1]);
    }
    //Check if user entered exec with at least one argument
    else if(strcmp("exec", input_words[0]) == 0 && (strcmp("", input_words[1]) != 0))
    {
        //run exec command
        run_exec(input_words[0], input_words);
    }
    //Check if user entered path or file
    else if('.' == input_words[0][0] || '/' == input_words[0][0])
    {
        //run exec_file command
        run_exec_file(input_words);
    }
    else if(strcmp("", input_words[0]) == 0)
    {
        
    }
    //Check for bad command
    else
    {
        //Check if non_path_func passes
        if(check_non_path_builtin(input_words, redirect_int, input_file, out_put_file))
        {

        }
        else
        {
            printf("Unrecognized command.\n");
        }
    }
}

//Function that checks if input_words has a < > in it and returns what index it finds it
int check_for_redirect(char input_words[256][256])
{
    //Counter to track which index we are at
    int counter = 0;

    //Iterate through all the input_words
    while(strcmp("", input_words[counter]) != 0)
    {
        //If any word is < or > return the counter which will tell us what index its at
        if(input_words[counter][0] == '<' || input_words[counter][0] == '>')
        {
            return counter;
        }
        //If it doesnt find it move to next index
        counter++;
    }

    //If it never finds a < or > return -1
    return -1;
}













































