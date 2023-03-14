/*
 * Loader Implementation
 *
 * 2022, Operating Systems
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#include "exec_parser.h"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

static so_exec_t *exec;
static struct sigaction default_handler;
static int file_descriptor;


static void segv_handler(int signum, siginfo_t *info, void *context)
{
	if (signum != SIGSEGV || info->si_signo != SIGSEGV)
		goto default_handler_label;

	int page_size = getpagesize();
	int found_address = 0;

	for (int i = 0; i < exec->segments_no && !found_address; i++) {
		so_seg_t *segment = &(exec->segments[i]);
		int segment_pages = segment->mem_size / page_size;
		int segment_filled_pages = segment->file_size / page_size;
		int segment_start_page = segment->vaddr / page_size * page_size;
		
		// gasesc segementul care contine adresa page fault-ului
		if ((int)info->si_addr >= segment->vaddr && (int)info->si_addr < segment->vaddr + segment->mem_size) {
			found_address = 1;

			if (segment->data == NULL)
				segment->data = calloc(segment_pages, sizeof(int));

			// gasesc pagina din segment unde se afla adresa page fault-ului
			int page_index = 0;

			while (!((int)info->si_addr >= segment_start_page + page_size * page_index &&
			(int)info->si_addr < segment_start_page + page_size * (page_index + 1))) {
				page_index++;
			}
			
			// adresa de inceput a paginii
			int page_address = segment->vaddr + page_size * page_index;

			// verific daca pagina este deja mapata
			int already_mapped = 0;

			if (((int *)(segment->data))[page_index] == 1)
				already_mapped = 1;

			if (already_mapped)
				goto default_handler_label;

			// mapez pagina, initial este zeroizata
			int *ptr = mmap((int *)page_address, page_size, PROT_WRITE, MAP_FIXED | MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

			if (ptr == MAP_FAILED)
				goto default_handler_label;
				
			// marchez pagina ca fiind mapata
			((int *)segment->data)[page_index] = 1;

			// verific daca am iesit in afara paginilor care contin date
			if (segment_filled_pages < page_index) 
				goto permissions_label;

			// aduc cursorul la adresa de inceput a paginii
			if (lseek(file_descriptor, segment->offset + page_index * page_size, SEEK_SET) < 0) 
				goto default_handler_label;

			// copiez datele din fisier in pagina mapata
			if (read(file_descriptor, ptr, MIN(page_size, segment->file_size - page_index * page_size)) < 0)
				goto default_handler_label;

			// aplic permisiuni pe pagina
permissions_label:
			mprotect(ptr, page_size, segment->perm);
			return;			
		}
	}

default_handler_label:
	default_handler.sa_sigaction(signum, info, context);
	return;
}

int so_init_loader(void)
{
	int rc;
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = segv_handler;
	sa.sa_flags = SA_SIGINFO;
	rc = sigaction(SIGSEGV, &sa, &default_handler);
	if (rc < 0) {
		perror("sigaction");
		return -1;
	}
	return 0;
}

int so_execute(char *path, char *argv[])
{
	file_descriptor = open(path, O_RDONLY);
	if (file_descriptor < 0)
		return -1;

	exec = so_parse_exec(path);
	if (!exec)
		return -1;

	so_start_exec(exec, argv);

	close(file_descriptor);

	return -1;
}
