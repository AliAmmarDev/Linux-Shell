# Introduction

The shell is a command language interpreter that executes commands read from the command line or shell scripts from files. Each operating system consists of one or more shells. The shell communicates with the system kernel which is the core of the operating system and does all the memory management, task scheduling and I/O handling. The following shell was written in C and developed and tested on Linux CentOS 7. 

# Features

This custom shell supports various built-in commands. The shell also supports executing programs as foreground and background processes. Programs can also be executed with parameters. The shell also supports pipe-lining two processes and redirecting program output to files.

## Assumptions

-   A single line of input is a maximum of 150 characters long.
-   Each token is no more than 20 characters long.
-   A path string is no more than 1024 characters long.

## Commands

**info**  

Running this command will simply print some information regarding the author of the shell.

**exit**

This command calls the C function exit() which terminates the program immediately.

**pwd**

This command makes a system call to getcwd() which copies the pathname of the current working directory to the array pointed to by currentDirectory. If the path exceeds the size of 1024 bytes, -1 is returned. The array containing the path could be generated straight inside a print statement however if this is the case, every time the system call is made 1024 will be allocated and never freed. To solve this, the array is stored in an array that temp is pointing to which is then freed after the print statement.

**cd**

This command makes a system call to chdir(). This changes the current working directory to the directory specified in the argument (args[1]). If cd is entered with no parameters it will change directory to the home directory using the getenv() system call. This searches for the directory with the environment name 'HOME'.  

**clear**

This command will print '\e[1;1H\e[2J' which clears the screen. This is an American National Standards Institute (ANSI) escape sequence. '\e' is the escape character. ' e[1;1H' relocates the cursor to position (1,1) and '\e[2J' clears the screen.

**mkdir**

mkdir makes a system call to mkdir() which creates a directory(ies) of the names given as parameters. Mkdir will print an error if the directory 'name' already exists. The second argument given in mkdir() is the appropriate permission the newly created directory shall have.  

**rmdir**

Rmdir calls the rmdir command which removes a directory(ies) given as parameters. An error is printed if the directory being removed does not exist.

# Launching processes

When launching a program, the process is represented with a process ID. A system call is made to fork() which duplicates the current process (shell). The duplicated process is the child process. If forking is successful (ie. child process ID = 0) the program launches successfully.

All programs in this shell are launched using the system call execvp() which allows the child process to run a different program to the parent process. When this execvp() runs, the program file specified in the first parameter is loaded into the callers address space and over-writes the program there. The second argument is a pointer to an array of strings which are passed to the program on execution. The exec() family consists of several system calls. Characters specified after exec in the call have slightly different functionalities:

**i**  : arguments are passed as a list of strings.
**v** : arguments are passed as an array of strings
**p** : path to search for new program
**e** : the environment can be specified by the caller

## Foreground process (ex)
Launching a program as a foreground process will block the use of the shell for the duration of the process. This block is implemented with the system call waitpid(). This stops the parent process (shell) from continuing until the child process has terminated. This can also be implemented using the system call wait(). The difference is wait() waits for the first child process to terminate whereas waitpid() waits for a specific child process mentioned in the first argument. execvp() takes in an array of strings as the second parameter. So if a program requires parameters it can be launched with no issues.

## Background process (exb)
This will launch the program as a background process. Launching the program as a background process means the shell will execute the command while making the shell still available for interaction. This means there is no reason to call to waitpid() since the parent program can instantly continue after launching the program.


## Pipe-lining
Pipe-lining is a way of manipulating data such that the output of one program can be directly used as input for another program. Pipe-lining is denoted by '|' and is executed in the format ex A | ex B where the standard output of A is used as standard input for B. This shell only supports the use of one pipe so only 2 programs may run consecutively.

A temporary array is made where the arguments are stored which will later be executed. A pipe is created from the system call pipe(). This takes 2 file descriptors as a parameter in which fileDescriptor(0) is used as the read end of the pipe and fileDescriptor(1) is used as the write end of the pipe. The data that is written in fileDescriptor1 is buffered by the kernel until it is read from the other end of the pipe.

A process is made with fork() in which the program left of the pipe is to be launched (ex A). When this process is launched, a system call is made to dup2() where the standard output of A is copied into the file descriptor(1). After the program launches the read end of the pipe is closed because the other process connected by the pipe also has access to the write end which is undesirable.

Before the second process can be launched it must wait until the first process has terminated. This is implemented with the waitpit() system call.

Another process is made using fork(). In this process, the program takes in the data stored in fileDescriptor(0) as standard input using dup2(). After the program launches fileDescriptor(1) is closed and the shell waits for the process to end before it can continue taking in input.

## Output redirection
Standard output of programs can be redirected to files using '>'. The output source is referred to by the integer fileDescriptor. In this shell, fileDescriptor will have value 3 because a new file is always being opened. This value is returned by the call open(). In this call, open() takes in 2 parameters.  The first is the path name of the output file. Flags are provided in between to specify the file mode. 'O_CREAT' will create the file if the file does not exist. 'O_TRUNC'  means if the file exists it will be reset to length 0. 'O_WRONLY'  will open the file for writing only. The permission to the file is given.  

After the program executes the standard output is copied into the file descriptor using dup2(). This function is executed as a foreground process. Another way to implement this function is to use fopen() or freopen() which work on a higher level than open(). They return a pointer to a FILE stream and can be used by the stdio library in c and can work on multiple platforms. Open() is a system call specific to UNIX and will not work on platforms that do not support unix. Since this is an implementation for a linux shell, open() was the better option.

# Error handling

If for whatever reason a process fails to run, the process is terminated using a system call to kill(). The kill command sends a signal to the process which are processed by signal handlers. In these cases, the signal sent is SIGTERM which is signal used to cause program termination. SIGTERM is used rather than SIGKILL as SIGTERM can be handled by the process. SIGKILL terminates the process without the process even knowing as the signal is sent straight to initialisation. Relevant print statements have also been provided where needed.

# Drawbacks

Output of built in commands cannot be redirected to files using '>'. This is because when execvp() is called, it launches the default built in commands and not the ones implemented in this shell.

Pipelining and redirecting output do not work simultaneously because these functions are being called in an if else manner.
