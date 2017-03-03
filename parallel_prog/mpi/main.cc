#include <cassert>
#include <cstring>

#include <mpi.h>
#include <gflags/gflags.h>

#include "page.h"
#include "io.h"
#include "logging.h"
#include "root.h"

DEFINE_string(dataset, "data/test", "Input dataset");
DEFINE_int32(chunk_size, 2, "Number of pages per chunk");
DEFINE_double(damping_factor, 0.85, "Param for PageRank");
DEFINE_double(eps, 1e-7, "Computation precision");

// Contains variables shared across processes. We need to sync them all the time.
// That's why size of data here should be as minimal as possible.
namespace world {
int size;
int page_cnt;
int pages_per_proc;
double *pr;
int *out_link_cnts;
bool go_on;
}  // namespace world

// Variables that are specific for each processor.
namespace proc {
int rank;
int page_cnt;
Page *pages;
double *pr;
}  // namespace proc

// These data are only relevant to root processor. So there should not be general
// code that tries to access root data. Use if (rank == 0) {...} guard everywhere.
namespace root {
int dangling_page_cnt;
int *dangling_pages;
double *old_pr;
}  // namespace root

void EvaluatePr() {
	for (int i = 0; i < proc::page_cnt; i++) {
		Page &page = proc::pages[i];
		double pr = 0;
		for (int j = 0; j < page.in_link_cnt; j++) {
			int from_page = page.in_links[j];
			pr += world::pr[from_page] / world::out_link_cnts[from_page];
		}
		proc::pr[i] = pr;
	}
}

int main(int argc, char *argv[]) {
	using proc::rank;
	gflags::ParseCommandLineFlags(&argc, &argv, true /* remove_flags */);

	MPI_Init(nullptr, nullptr);
	MPI_Comm_size(MPI_COMM_WORLD, &world::size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	if (rank == 0) {
		VLOG(world::size);
	}

	if (rank == 0) {
		InitMetadata();
	}
	MPI_Bcast(&world::page_cnt, 1, MPI_INT, 0, MPI_COMM_WORLD);

	if (rank > 0) {
		world::out_link_cnts = new int[world::page_cnt];
	}
	MPI_Bcast(world::out_link_cnts, world::page_cnt, MPI_INT, 0, MPI_COMM_WORLD);

	world::pages_per_proc = world::page_cnt / world::size;
	proc::pages = new Page[world::pages_per_proc];
	int begin = world::pages_per_proc * rank;
	int end = begin + world::pages_per_proc - 1;
	ReadPages(begin, end, proc::pages);

	proc::page_cnt = world::pages_per_proc;
	// If world::page_cnt % world::size != 0 then there are some pages remaining.
	// We are going to process them at root processor.
	if (rank == 0) {
		ReadReminderPages();
	}

	for (int i = 0; i < proc::page_cnt; i++) {
		VLog("id", proc::pages[i].id);
	}

	world::pr = new double[world::page_cnt];
	if (rank == 0) {
		InitWorldPr();
	}
	proc::pr = new double[proc::page_cnt];
	if (rank == 0) {
		root::old_pr = new double[world::page_cnt];
	}
	world::go_on = true;
	int step = 0;
	while (world::go_on) {
		MPI_Bcast(world::pr, world::page_cnt, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		if (rank == 0) {
			memcpy(root::old_pr, world::pr, world::page_cnt * sizeof(double));
		}

		EvaluatePr();
		// NOTE! We use world::pages_per_proc instead of proc::page_cnt
		// to gather PRs. It's because size should remaind the same for
		// all the processes. Some part of PRs evaluated at root are not
		// going to be visible for MPI_Gather. But, it's OK until they
		// remain on host. We are going to account them later.
		MPI_Gather(proc::pr, world::pages_per_proc, MPI_DOUBLE,
				world::pr, world::pages_per_proc, MPI_DOUBLE, 0, MPI_COMM_WORLD);

		if (rank == 0) {
			AddRemainderPr();
			AddDanglingPagesPr();
			AddRandomJumpsPr();

			double err = EvaluateError();
			world::go_on = err > FLAGS_eps;
		}

		MPI_Bcast(&world::go_on, 1, MPI_CHAR, 0, MPI_COMM_WORLD);
		step++;
	}
	if (rank == 0) {
		VLOG(step);
		VLog("PageRank", world::pr, world::page_cnt);
	}

	delete[] proc::pr;
	delete[] proc::pages;
	delete[] world::pr;
	delete[] world::out_link_cnts;
	if (rank == 0) {
		delete[] root::old_pr;
		delete[] root::dangling_pages;
	}

	MPI_Finalize();
	gflags::ShutDownCommandLineFlags();
	return 0;
}