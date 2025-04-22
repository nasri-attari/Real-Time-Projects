#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "config.h"

int main() {
	Config config;

	// Load configuration
	if (load_config(CONFIG_FILE, &config) != 0) {
		fprintf(stderr, "[Main] Failed to load configuration file.\n");
		return 1;
	}

	printf("[Main] max_time = %d | gui_enabled = %d\n", config.max_time, config.gui_enabled);

	// Create GUI FIFO
	if (mkfifo(FIFO_GUI, 0666) == -1 && errno != EEXIST) {
		perror("[Main] Failed to create GUI FIFO");
		return 1;
	}

	// Fork GUI if enabled
	if (config.gui_enabled) {
		pid_t gui_pid = fork();
		if (gui_pid == 0) {
			execlp("./bin/gui", "gui", CONFIG_FILE, NULL);  // <-- Pass config file
			perror("execlp failed for GUI");
			exit(EXIT_FAILURE);
		}
		printf("[Main] GUI launched with PID %d\n", gui_pid);
	}


	// Fork referee
	pid_t referee_pid = fork();
	if (referee_pid == 0) {
		execlp("./bin/referee", "referee", NULL);
		perror("execlp failed for referee");
		exit(EXIT_FAILURE);
	}

	printf("[Main] Referee launched with PID %d\n", referee_pid);

	// Wait for referee
	waitpid(referee_pid, NULL, 0);

	printf("[Main] Simulation ended.\n");
	return 0;
}
