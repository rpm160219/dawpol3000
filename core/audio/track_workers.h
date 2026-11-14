// Persistent helpers for independent track renders. Only start/stop may allocate.
// Audio owns each batch; every helper is joined at its sample/event boundary.
#pragma once
#include <atomic>
#include <cstdint>
#include <cfenv>
#include <pthread.h>
#include <semaphore.h>
namespace dp {
class TrackWorkers {
  public:
    static constexpr int MAX_HELPERS = 3, MAX_JOBS = 32;
    using Task = void (*)(void *, int);
    ~TrackWorkers() { stop(); }
    // totalThreads: 0 = hardware/env auto, 1 = serial; capped at four.
    void start(int totalThreads = 0);
    void stop(); // only after callback has stopped
    int threads() const { return helpers_ + 1; }
    void begin(Task task, void *context, const int *jobs, int count, bool hasSerialWork);
    void prepareCallback(); // once on the actual audio thread; best-effort RT priority
    void finish();          // caller also consumes unclaimed jobs, then joins helpers
  private:
    struct Helper {
        TrackWorkers *owner = nullptr;
        pthread_t thread{};
        sem_t wake{};
    };
    Helper helper_[MAX_HELPERS];
    int helpers_ = 0, posted_ = 0, count_ = 0, policy_ = 0, priority_ = 0;
    int jobs_[MAX_JOBS] = {};
    Task task_ = nullptr;
    void *context_ = nullptr;
    bool priorityCaptured_ = false;
    fenv_t floatingPoint_{};
    std::atomic<bool> stopping_{false};
    alignas(64) std::atomic<uint32_t> epoch_{0};
    alignas(64) std::atomic<uint32_t> next_{0};
    alignas(64) std::atomic<uint32_t> done_{0};
    void consume();
    static void *run(void *);
};
} // namespace dp
