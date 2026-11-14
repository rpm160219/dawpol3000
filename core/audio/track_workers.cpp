#include "audio/track_workers.h"
#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <linux/futex.h>
#include <sched.h>
#include <sys/syscall.h>
#include <unistd.h>
namespace dp {
namespace {
static_assert(std::atomic<uint32_t>::is_always_lock_free, "audio counters must be lock-free");
static_assert(sizeof(std::atomic<uint32_t>) == sizeof(uint32_t), "Linux futex word layout");
void waitCounter(std::atomic<uint32_t> &word, uint32_t expected) {
    // Compare-and-sleep is atomic in the kernel: a completion before this call
    // returns EAGAIN instead of losing the wake-up. EINTR is checked by caller.
    syscall(SYS_futex, reinterpret_cast<uint32_t *>(&word), FUTEX_WAIT_PRIVATE, expected, nullptr, nullptr,
            0);
}
void wakeCounter(std::atomic<uint32_t> &word) {
    syscall(SYS_futex, reinterpret_cast<uint32_t *>(&word), FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0);
}
void relax() {
#if defined(__aarch64__) || defined(__arm__)
    __asm__ __volatile__("yield");
#elif defined(__x86_64__) || defined(__i386__)
    __asm__ __volatile__("pause");
#endif
}
} // namespace
void TrackWorkers::start(int total) {
    stop();
    if (total == 0) {
        const char *s = std::getenv("DAWPOL_AUDIO_THREADS");
        if (s)
            total = std::atoi(s);
    }
    if (total <= 0) {
        long cpus = sysconf(_SC_NPROCESSORS_ONLN);
        total = (int)std::max(1L, cpus);
    }
    total = std::clamp(total, 1, MAX_HELPERS + 1);
    stopping_.store(false, std::memory_order_relaxed);
    priorityCaptured_ = false;
    for (int i = 0; i < total - 1; i++) {
        Helper &h = helper_[i];
        h.owner = this;
        if (sem_init(&h.wake, 0, 0))
            break;
        if (pthread_create(&h.thread, nullptr, &TrackWorkers::run, &h)) {
            sem_destroy(&h.wake);
            break;
        }
        ++helpers_;
    }
}
void TrackWorkers::stop() {
    finish();
    stopping_.store(true, std::memory_order_release);
    for (int i = 0; i < helpers_; i++)
        sem_post(&helper_[i].wake);
    for (int i = 0; i < helpers_; i++) {
        pthread_join(helper_[i].thread, nullptr);
        sem_destroy(&helper_[i].wake);
    }
    helpers_ = 0;
}
void TrackWorkers::prepareCallback() {
    if (priorityCaptured_)
        return;
    sched_param p{};
    if (!pthread_getschedparam(pthread_self(), &policy_, &p)) {
        // A sleeping real-time worker must be scheduled promptly when the
        // callback wakes it. On unprivileged hosts failure simply keeps the
        // scheduler already selected by SDL/the OS. No system-wide changes.
        if (policy_ == SCHED_OTHER) {
            sched_param rt{};
            rt.sched_priority = 8;
            if (!pthread_setschedparam(pthread_self(), SCHED_FIFO, &rt)) {
                policy_ = SCHED_FIFO;
                p = rt;
            }
        }
        priority_ = p.sched_priority;
    }
    std::fegetenv(&floatingPoint_);
    priorityCaptured_ = true;
}
void TrackWorkers::begin(Task task, void *context, const int *jobs, int count, bool serial) {
    finish();
    // Prior finish() guarantees no helper can still read the old descriptor.
    task_ = task;
    context_ = context;
    count_ = std::clamp(count, 0, MAX_JOBS);
    for (int i = 0; i < count_; i++)
        jobs_[i] = jobs[i];
    next_.store(0, std::memory_order_relaxed);
    done_.store(0, std::memory_order_relaxed);
    posted_ = std::min(helpers_, std::max(0, count_ - (serial ? 0 : 1)));
    if (posted_)
        prepareCallback();
    epoch_.fetch_add(1, std::memory_order_release);
    for (int i = 0; i < posted_; i++)
        sem_post(&helper_[i].wake);
}
void TrackWorkers::consume() {
    for (;;) {
        uint32_t i = next_.fetch_add(1, std::memory_order_relaxed);
        if (i >= (uint32_t)count_)
            break;
        task_(context_, jobs_[i]);
    }
}
void TrackWorkers::finish() {
    if (!task_)
        return;
    consume();
    // Short completions remain in userspace. A longer join yields the CPU to
    // UI/other workers; this barrier has no mutex and never abandons a live job.
    for (int spin = 0; spin < 64 && done_.load(std::memory_order_acquire) < (uint32_t)posted_; spin++)
        relax();
    for (;;) {
        uint32_t n = done_.load(std::memory_order_acquire);
        if (n >= (uint32_t)posted_)
            break;
        waitCounter(done_, n);
    }
    task_ = nullptr;
    context_ = nullptr;
    posted_ = 0;
    count_ = 0;
}
void *TrackWorkers::run(void *arg) {
    Helper &h = *static_cast<Helper *>(arg);
    TrackWorkers &p = *h.owner;
#if defined(__linux__)
    pthread_setname_np(pthread_self(), "dp-audio");
#endif
    int policy = -1, priority = -1;
    bool fpSet = false;
    for (;;) {
        while (sem_wait(&h.wake)) {
            if (errno != EINTR)
                return nullptr;
        }
        if (p.stopping_.load(std::memory_order_acquire))
            break;
        p.epoch_.load(std::memory_order_acquire);
        if (!fpSet) {
            std::fesetenv(&p.floatingPoint_);
            fpSet = true;
        }
        if (policy != p.policy_ || priority != p.priority_) {
            sched_param sp{};
            sp.sched_priority = p.priority_;
            pthread_setschedparam(pthread_self(), p.policy_, &sp); // best effort, outside callback
            policy = p.policy_;
            priority = p.priority_;
        }
        p.consume();
        const uint32_t expected = (uint32_t)p.posted_;
        const uint32_t completed = p.done_.fetch_add(1, std::memory_order_acq_rel) + 1;
        if (completed == expected)
            wakeCounter(p.done_);
    }
    return nullptr;
}
} // namespace dp
