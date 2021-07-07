#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/user.h>
#include <wait.h>


#include "utils.h"

int main(){

if(argc < 4)
	{
		usage(argv[0]);
		return 1;
	}

	char* command = argv[1];
	char* commandArg = argv[2];
	char* libname = argv[3];
	char* libPath = realpath(libname, NULL);

	char* processName = NULL;
	pid_t target = 0;

	if(!libPath)
	{
		fprintf(stderr, "can't find file \"%s\"\n", libname);
		return 1;
	}

	if(!strcmp(command, "-n"))
	{
		processName = commandArg;
		target = findProcessByName(processName);
		if(target == -1)
		{
			fprintf(stderr, "doesn't look like a process named \"%s\" is running right now\n", processName);
			return 1;
		}

		printf("targeting process \"%s\" with pid %d\n", processName, target);
	}
	else if(!strcmp(command, "-p"))
	{
		target = atoi(commandArg);
		printf("targeting process with pid %d\n", target);
	}
	else
	{
		usage(argv[0]);
		return 1;
	}

	int libPathLength = strlen(libPath) + 1;

	int mypid = getpid();
	long mylibcaddr = getlibcaddr(mypid);

	// find the addresses of the syscalls that we'd like to use inside the
	// target, as loaded inside THIS process (i.e. NOT the target process)
	long mallocAddr = getFunctionAddress("malloc");
	long freeAddr = getFunctionAddress("free");
	long dlopenAddr = getFunctionAddress("__libc_dlopen_mode");
	long raiseAddr = getFunctionAddress("raise");

	// use the base address of libc to calculate offsets for the syscalls
	// we want to use
	long mallocOffset = mallocAddr - mylibcaddr;
	long freeOffset = freeAddr - mylibcaddr;
	long dlopenOffset = dlopenAddr - mylibcaddr;
	long raiseOffset = raiseAddr - mylibcaddr;

	// get the target process' libc address and use it to find the
	// addresses of the syscalls we want to use inside the target process
	long targetLibcAddr = getlibcaddr(target);
	long targetMallocAddr = targetLibcAddr + mallocOffset;
	long targetFreeAddr = targetLibcAddr + freeOffset;
	long targetDlopenAddr = targetLibcAddr + dlopenOffset;
	long targetRaiseAddr = targetLibcAddr + raiseOffset;

	struct user_regs oldregs, regs;
	memset(&oldregs, 0, sizeof(struct user_regs));
	memset(&regs, 0, sizeof(struct user_regs));

	ptrace_attach(target);

	ptrace_getregs(target, &oldregs);
	memcpy(&regs, &oldregs, sizeof(struct user_regs));

	// find a good address to copy code to
	long addr = freespaceaddr(target) + sizeof(long);

	// now that we have an address to copy code to, set the target's
	// program counter to it.
	//
	// subtract 4 bytes from the actual address, because ARM's PC actually
	// refers to the next instruction rather than the current instruction.
	regs.uregs[15] = addr - 4;

	// pass arguments to my function injectSharedLibrary() by loading them
	// into the right registers. see comments in injectSharedLibrary() for
	// more details.
	regs.uregs[1] = targetRaiseAddr;
	regs.uregs[2] = targetMallocAddr;
	regs.uregs[3] = targetDlopenAddr;
	regs.uregs[4] = targetFreeAddr;
	regs.uregs[5] = libPathLength;
	ptrace_setregs(target, &regs);


    return 0;
}