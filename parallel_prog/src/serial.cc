#include <cstring>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "lib/page.h"
#include "lib/io.h"
#include "lib/math.h"
#include "lib/pagerank.h"
#include "lib/utils.h"
#include "lib/benchmark.h"

DEFINE_string(dataset, "data/generated/test", "Input dataset");
DEFINE_double(damping_factor, 0.85, "PageRank main parameter");
DEFINE_double(eps, 1e-7, "Computation precision");
DEFINE_string(output, "/home/lionell/dev/labs/parallel_prog/out/serial",
		"Path to file where to store calculated PageRank");

int main(int argc, char *argv[]) {
	FLAGS_logtostderr = 1;
	google::ParseCommandLineFlags(&argc, &argv, true /* remove_flags */);
	google::InitGoogleLogging(argv[0]);
	Timer timer;

	int page_cnt, chunk_size;
	ReadPageCount(FLAGS_dataset, &page_cnt);
	ReadChunkSize(FLAGS_dataset, &chunk_size);

	int *out_link_cnts = new int[page_cnt];
	ReadOutLinkCounts(FLAGS_dataset, page_cnt, out_link_cnts);

	timer.Start();
	Page *pages = new Page[page_cnt];
	ReadPages(FLAGS_dataset, chunk_size, 0, page_cnt - 1, pages);
	timer.StopAndReport("Reading pages");

	int dangling_page_cnt, *dangling_pages;
	ExploreDanglingPages(out_link_cnts, page_cnt,
			&dangling_page_cnt, &dangling_pages);

	double *pr = new double[page_cnt];
	InitPr(pr, page_cnt);
	double *old_pr = new double[page_cnt];

	timer.Start();
	bool go_on = true;
	int step = 0;
	while (go_on) {
		memcpy(old_pr, pr, page_cnt * sizeof(double));

		EvaluatePr(pages, page_cnt, old_pr, out_link_cnts, pr);
		AddDanglingPagesPr(old_pr, page_cnt, dangling_pages, dangling_page_cnt, pr);
		AddRandomJumpsPr(pr, page_cnt);

		double err = L1Norm(pr, old_pr, page_cnt);
		go_on = err > FLAGS_eps;
		step++;
	}
	timer.StopAndReport("PageRank");

	LOG(INFO) << "Number of steps: " << step;
	WritePrToFile(FLAGS_output, pr, page_cnt);

	delete[] old_pr;
	delete[] pr;
	delete[] pages;
	delete[] dangling_pages;
	delete[] out_link_cnts;

	google::ShutDownCommandLineFlags();
	return 0;
}