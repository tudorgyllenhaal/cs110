#include <cassert>
#include <ctime>
#include <cctype>
#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>
#include <sched.h>
#include "subprocess.h"

using namespace std;

struct worker {
	worker() {}
	worker(char *argv[]) : sp(subprocess(argv, true, false)), available(false) {}
	subprocess_t sp;
	bool available;
};

static const size_t kNumCPUs = sysconf(_SC_NPROCESSORS_ONLN);
static vector<worker> workers(kNumCPUs);
static size_t numWorkersAvailable = 0;

static void markWorkersAsAvailable(int sig) {
}

static const char *kWorkerArguments[] = {"./factor.py", "--self-halting", NULL};
static void spawnAllWorkers() {
}
static size_t getAvailableWorker() {
}

static void broadcastNumbersToWorkers() {
}

static void waitForAllWorkers() {
}

static void closeAllWorkers() {
}

int main(int argc, char *argv[]) {
	signal(SIGCHLD, markWorkersAsAvailable);
	spawnAllWorkers();
	broadcastNumbersToWorkers();
	waitForAllWorkers();
	closeAllWorkers();
	return 0;
}
