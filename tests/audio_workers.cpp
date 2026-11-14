#include "audio/track_workers.h"
#include <atomic>
#include <cfenv>
#include <cmath>
#include <cstdio>
#include <cstdlib>
using namespace dp;
static void check(bool ok, const char *text) {
    if (!ok) {
        std::fprintf(stderr, "FAIL %s\n", text);
        std::exit(1);
    }
}
struct Tasks {
    int hits[32] = {};
    float rounded[32] = {};
    std::atomic<int> helpers{0};
    pthread_t caller{};
};
static void work(void *ctx, int job) {
    auto &t = *static_cast<Tasks *>(ctx);
    ++t.hits[job];
    volatile float x = 1.75f;
    t.rounded[job] = std::nearbyint(x);
    if (!pthread_equal(t.caller, pthread_self()))
        t.helpers.fetch_add(1);
    volatile unsigned sum = 0;
    for (unsigned i = 0; i < 20000; i++)
        sum += i;
}
static void poolChecks() {
    for (int threads = 1; threads <= 4; threads++) {
        TrackWorkers pool;
        pool.start(threads);
        Tasks t;
        t.caller = pthread_self();
        int ids[32];
        for (int i = 0; i < 32; i++)
            ids[i] = i;
        std::fesetround(FE_DOWNWARD);
        for (int batch = 0; batch < 300; batch++) {
            int n = batch % 33;
            for (int i = 0; i < 32; i++) {
                t.hits[i] = 0;
                t.rounded[i] = 0;
            }
            pool.begin(work, &t, ids, n, batch % 2);
            pool.finish();
            for (int i = 0; i < 32; i++) {
                check(t.hits[i] == (i < n), "each task executes exactly once before join");
                if (i < n)
                    check(t.rounded[i] == 1, "worker inherits callback floating-point environment");
            }
        }
        check(pool.threads() == 1 || t.helpers.load() > 0, "helpers actually executed tasks");
        std::fesetround(FE_TONEAREST);
        pool.stop();
        pool.start(threads);
        pool.stop();
    }
    std::puts("PASS pool: 1..4 threads, idle/busy wake-ups, unique tasks, barriers, FPU, restart/shutdown");
}
int main() { poolChecks(); }
