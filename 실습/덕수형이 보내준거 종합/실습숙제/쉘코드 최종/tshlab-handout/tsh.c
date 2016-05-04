/* 
 * tsh - A tiny shell program with job control
 * 
 * <Put your name and login ID here>
 *@id: 200701960
 *@name: KimDeakSoo
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>

/* Misc manifest constants */
#define MAXLINE    1024   /* max line size */
#define MAXARGS     128   /* max args on a command line */
#define MAXJOBS      16   /* max jobs at any point in time */
#define MAXJID    1<<16   /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/* 
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Global variables */
extern char **environ;      /* defined in libc */
char prompt[] = "tsh> ";    /* command line prompt (DO NOT CHANGE) */
int verbose = 0;            /* if true, print additional output */
int nextjid = 1;            /* next job ID to allocate */
char sbuf[MAXLINE];         /* for composing sprintf messages */

struct job_t {              /* The job struct */
    pid_t pid;              /* job PID */
    int jid;                /* job ID [1, 2, ...] */
    int state;              /* UNDEF, BG, FG, or ST */
    char cmdline[MAXLINE];  /* command line */
};
struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */


/* Function prototypes */
void eval(char *cmdline);
int builtin_cmd( char** apszBuffer );
void waitfg( pid_t pid, int output_fd );

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv); 
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs); 
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid); 
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid); 
int pid2jid(pid_t pid); 
void listjobs(struct job_t *jobs, int output_fd);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

/*
 * main - The shell's main routine 
 */
int main(int argc, char **argv) 
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
	switch (c) {
	case 'h':             /* print help message */
	    usage();
	    break;
	case 'v':             /* emit additional diagnostic info */
	    verbose = 1;
	    break;
	case 'p':             /* don't print a prompt */
	    emit_prompt = 0;  /* handy for automatic testing */
	    break;
	default:
	    usage();
	}
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT,  sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler);  /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler);  /* Terminated or stopped child */
    Signal(SIGTTIN, SIG_IGN);
    Signal(SIGTTOU, SIG_IGN);

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler); 

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1) {

	/* Read command line */
	if (emit_prompt) {
	    printf("%s", prompt);
	    fflush(stdout);
	}
	if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
	    app_error("fgets error");
	if (feof(stdin)) { /* End of file (ctrl-d) */
	    fflush(stdout);
	    fflush(stderr);
	    exit(0);
	}

	/* Evaluate the command line */
	eval(cmdline);
	fflush(stdout);
	fflush(stdout);
    } 

    exit(0); /* control never reaches here */
}

/* 
 * eval - Evaluate the command line that the user has just typed in
 * 
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
 */


void eval(char *cmdline) 
{
	int i; // Loop Index
	int nFD, nFD2; // File Discriptor�� ����Ǵ� ����
	int nState = 0;	// FG, BG, ST�� ����Ǵ� ����
	char *apszBuffer[MAXARGS] = { 0x00 }; // parseline�� ����� ����Ǵ� �迭
	pid_t pid; // pid ���� �޾ƿ��� ���� ����
	sigset_t mask; // Signal�� �ޱ� ���� ����

	nState = parseline( cmdline, (char**)apszBuffer ); // �Էµ� cmdline�� parseline�Ͽ� BG�ϰ�� nState�� 1�� �����


	if( !builtin_cmd( apszBuffer ) ) // apszBuffer�� ���� builtin_cmd�� ������ ����
	{
		if(sigemptyset(&mask) < 0) // Signal�� �߰�
			unix_error("sigemptyset error");
		if(sigaddset(&mask, SIGCHLD))
			unix_error("sigaddset error");
		if(sigaddset(&mask, SIGINT))
			unix_error("sigaddset error");
		if(sigaddset(&mask, SIGTSTP))
			unix_error("sigaddset error");
		if(sigprocmask(SIG_BLOCK, &mask, NULL) < 0)
			unix_error("sigprocmask error");

		if( !( pid = fork() ) ) // builtin_cmd�� ���� ��� fork�Ͽ� child process�� ����
		{
			sigprocmask(SIG_UNBLOCK, &mask, NULL); // signal block�� Ǯ���ش�.

			if(setpgid(0, 0) < 0) { unix_error("setpgid error"); }

			for(i = 0; apszBuffer[i]; i++) // parse�� ���� �˻�
			{
				if(*apszBuffer[i] == '<') // ����ڰ� �Է��� ��� �� '<'�� �ִٸ�
				{
					nFD = open(apszBuffer[i + 1], O_RDONLY); // nFD�� open�� ������ ��ũ���͸� ����(�б� �������� open)
					dup2(nFD, 0); // ���� ��ũ���͸� ǥ�� �Է¿� ����
				}

				else if(*apszBuffer[i] == '>') // ����ڰ� �Է��� ��� �� '>'�� �ִٸ�
				{
					nFD2 = open(apszBuffer[i + 1], O_WRONLY); // nFD�� open�� ������ ��ũ���͸� ����(���� �������� open)
					dup2(nFD2, 1); // ���� ��ũ���͸� ǥ�� ��¿� ����
				}
			}

			if( execve(apszBuffer[0], apszBuffer, environ) < 0 ) // child process�� apszBuffer�ȿ� �ִ� ��ɾ ������ process�� �����
			{
				printf("%s : Command not found.\n", apszBuffer[0]);
				exit(0);
			}
		}

		if(nFD != -1)  { close(nFD); } // open�� �������� �ʾҴٸ� ���� ��ũ���͸� �ݾ��ش�.

		addjob(jobs, pid, nState + 1, cmdline); // fork�� process�� ������ job�� �߰�

		sigprocmask(SIG_UNBLOCK, &mask, NULL);

		if(!nState) // nState�� FG��� FG job�� wait��Ŵ 
		{
			waitfg(pid, 1);
		}
		else { printf("[%d] (%d) %s", pid2jid(pid), pid, jobs[maxjid(jobs) - 1].cmdline); } // nState�� BG��� BG�� ��½�Ŵ
	}
}

int builtin_cmd( char** apszBuffer ) // built in ��ɾ���� �����ϰ� ����
{
	int i; // Loop Index
	int nFD, nFD2; // File Discriptor�� ����Ǵ� ����
	pid_t pid, jid; // pid�� jid�� ����Ǵ� ����

	if( !strcmp(apszBuffer[0], "quit") ) {exit(0);} // apszBuffer�� quit�� ����ִٸ� ���� �����Ŵ

	if( !strcmp(apszBuffer[0], "jobs") ) // apszBuffer�� jobs�� ����ִٸ�
	{
		for(i = 0; apszBuffer[i] != NULL; i++) // eval������ ���������� �ΰ��� ��Ȳ�� �����
		{
			if(*apszBuffer[i] == '<')
			{
				nFD = open(apszBuffer[i + 1], O_RDONLY);
				dup2(nFD, 0);
			}
			else if(*apszBuffer[i] == '>')
			{
				nFD2 = open(apszBuffer[i + 1], O_WRONLY); // ��½�Ű�� �ʱ� ���� dup2���� ����
			}
		}

		if(nFD2 != -1) // open�� �������� �ʾ����� 
		{
			listjobs(jobs, nFD2); // nFD�� �Ѱ� write���� ���� �����ϵ��� ��
		}
		else { listjobs(jobs, 1); } // ǥ�� ����� �Ѱ���

		if(nFD2 != 1) { close(nFD2); }

		return 1;
	}

	if(!strcmp(apszBuffer[0], "bg")) // apszBuffer�� "bg"�� ������
	{
		if(apszBuffer[1][0] == '%') // jid�� �ԷµǴ� ��쿣
		{
			jid = atoi(apszBuffer[1] + 1); // '%'�������ں��� ���ڷ� ��ȯ
			pid = getjobjid(jobs, jid)->pid; // �Էµ� jid�� pid�� ������
		}
		else { pid = atoi(apszBuffer[1]); }// pid�� �ԷµǴ� ��쿣 �ٷ� ���� 


		kill(-pid, SIGCONT); // ��� ���μ��� �׷쿡 SIGCONT �ñ׳��� ����

		jid = getjobpid(jobs, pid)->jid; // pid�� jid�� ������

		getjobpid(jobs, pid)->state = BG; // ���¸� BG�� ����

		printf("[%d] (%d) %s", jid, pid, getjobpid(jobs, pid)->cmdline);

		return 1;
	}

	if(!strcmp(apszBuffer[0], "fg")) // apszBuffer�� fg�� ������
	{
		if(apszBuffer[1][0] == '%') // bg�� ���� �����ϴ�
		{
			jid = atoi(apszBuffer[1] + 1);
			pid = getjobjid(jobs, jid)->pid;
		}
		else { pid = atoi(apszBuffer[1]); }

		kill(-pid, SIGCONT);

		getjobpid(jobs, pid)->state = FG; // ���¸� FG�� ����

		waitfg(pid, 1); // FG�̱� ������ wait��Ŵ

		return 1;
	}
	return 0;
}

void waitfg( pid_t pid, int output_fd )
{
	struct job_t *j = getjobpid(jobs, pid);
	char buf[MAXLINE];

	if(!j) { return; }

	while(j->pid == pid && j->state == FG ) { sleep(1); }
	if(verbose){
		memset(buf, '\0', MAXLINE);
		sprintf(buf, "waitfg : Process (%d) no longer the fg process:q\n", pid);
		if(write(output_fd, buf, strlen(buf)) < 0){
			fprintf(stderr, "Error writing to file\n");
			exit(1);
		}
	}
}

/* 
 * parseline - Parse the command line and build the argv array.
 * 
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a BG job, false if
 * the user hant j;
s requested a FG job.  
 */

int parseline(const char *cmdline, char **argv) 
{

    static char array[MAXLINE]; /* holds local copy of command line */
    char *buf = array;          /* ptr that traverses command line */
    char *delim;                /* points to first space delimiter */
    int argc;                   /* number of args */
    int bg;                     /* background job? */

    strcpy(buf, cmdline);
    buf[strlen(buf)-1] = ' ';     /* replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* ignore leading spaces */
	buf++;

    /* Build the argv list */
    argc = 0;
    if (*buf == '\'') {
	buf++;
	delim = strchr(buf, '\'');
    }
    else {
	delim = strchr(buf, ' ');
    }

    while (delim) {
	argv[argc++] = buf;
	*delim = '\0';
	buf = delim + 1;
	while (*buf && (*buf == ' ')) /* ignore spaces */
	    buf++;

	if (*buf == '\'') {
	    buf++;
	    delim = strchr(buf, '\'');
	}
	else {
	    delim = strchr(buf, ' ');
	}
    } 

    argv[argc] = NULL;

    if (argc == 0)  /* ignore blank line */
	return 1;

    /* should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0)
	argv[--argc] = NULL;

    return bg;

}


/*****************
 * Signal handlers
 *****************/

/* 
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.  
 */
void sigchld_handler(int sig) 
{
	int status;

	pid_t child_pid, jid;

	while((child_pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) // �ڽ� ���μ����� �ñ׳��� ���ö����� �ݺ�
	{
			jid = pid2jid( child_pid );  // jid���� �޾Ƶ���
			if(WIFSIGNALED(status)) // SIGNAL�� ���� ����� ���
			{ 
					printf("Job [%d] (%d) terminated by signal %d\n", jid, child_pid, WTERMSIG(status));
					deletejob(jobs, child_pid); // job�� �����Ѵ�.
			}
			else if(WIFSTOPPED(status)) // SIGNAL�� ���� ���� ������ ���
			{
					printf("Job [%d] (%d) stopped by signal %d\n", jid, child_pid, WSTOPSIG(status));
					jobs[jid - 1].state = ST; // ���¸� ST�� �����Ѵ�.
			}
			else if(WIFEXITED(status)) // �׳� ���� ����� ���
			{
					deletejob(jobs, child_pid); // ������ ���ش�.
			}
	}
}

/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */
void sigint_handler(int sig) 
{
	pid_t pid = fgpid(jobs); // FG�� job�� �޾ƿ�

	kill(-pid, SIGINT); // ��� ���μ��� �׷쿡 SIGINT����
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.  
 */
void sigtstp_handler(int sig) 
{
	pid_t pid = fgpid(jobs); // FG�� job�� �޾ƿ�
	
	kill(-pid, SIGTSTP); // ��� ���μ��� �׷쿡 SIGTSTP ����
}

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job) {
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	clearjob(&jobs[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *jobs) 
{
    int i, max=0;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid > max)
	    max = jobs[i].jid;
    return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline) 
{
    int i;

    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == 0) {
	    jobs[i].pid = pid;
	    jobs[i].state = state;
	    jobs[i].jid = nextjid++;
	    if (nextjid > MAXJOBS)
		nextjid = 1;
	    strcpy(jobs[i].cmdline, cmdline);
	    if(verbose){
		printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
	    }
	    return 1;
	}
    }
    printf("Tried to create too many jobs\n");
    return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *jobs, pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == pid) {
	    clearjob(&jobs[i]);
	    nextjid = maxjid(jobs)+1;
	    return 1;
	}
    }
    return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].state == FG)
	    return jobs[i].pid;
    return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t *getjobpid(struct job_t *jobs, pid_t pid) {
    int i;

    if (pid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid)
	    return &jobs[i];
    return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *jobs, int jid) 
{
    int i;

    if (jid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid == jid)
	    return &jobs[i];
    return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid) {
	    return jobs[i].jid;
	}
    return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *jobs, int output_fd) 
{
    int i;
    char buf[MAXLINE];

    for (i = 0; i < MAXJOBS; i++) {
	memset(buf, '\0', MAXLINE);
	if (jobs[i].pid != 0) {
	    sprintf(buf, "[%d] (%d) ", jobs[i].jid, jobs[i].pid);
	    if(write(output_fd, buf, strlen(buf)) < 0) {
		fprintf(stderr, "Error writing to output file\n");
		exit(1);
	    }
	    memset(buf, '\0', MAXLINE);
	    switch (jobs[i].state) {
	    case BG:
		sprintf(buf, "Running    ");
		break;
	    case FG:
		sprintf(buf, "Foreground ");
		break;
	    case ST:
		sprintf(buf, "Stopped    ");
		break;
	    default:
		sprintf(buf, "listjobs: Internal error: job[%d].state=%d ",
			i, jobs[i].state);
	    }
	    if(write(output_fd, buf, strlen(buf)) < 0) {
		fprintf(stderr, "Error writing to output file\n");
		exit(1);
	    }
	    memset(buf, '\0', MAXLINE);
	    sprintf(buf, "%s", jobs[i].cmdline);
	    if(write(output_fd, buf, strlen(buf)) < 0) {
		fprintf(stderr, "Error writing to output file\n");
		exit(1);
	    }
	}
    }
    if(output_fd != STDOUT_FILENO)
	close(output_fd);
}
/******************************
 * end job list helper routines
 ******************************/


/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void) 
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
    fprintf(stdout, "%s\n", msg);
    exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler) 
{
    struct sigaction action, old_action;

    action.sa_handler = handler;  
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
	unix_error("Signal error");
    return (old_action.sa_handler);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) 
{
    printf("Terminating after receipt of SIGQUIT signal\n");
    exit(1);
}
