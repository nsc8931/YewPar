#include "hpx/apply.hpp"
#include "hpx/runtime/actions/plain_action.hpp"
#include "hpx/runtime/naming/id_type.hpp"
#include "hpx/runtime/threads/executors/default_executor.hpp"

#include "Scheduler.hpp"
#include "ExponentialBackoff.hpp"

namespace Workstealing { namespace Scheduler {

void scheduler(hpx::util::function<void(), false> initialTask) {
  workstealing::ExponentialBackoff backoff;

  if (!local_policy) {
    std::cerr << "No local policy set when calling scheduler. Returning\n";
    return;
  }

  // If we are pre-initialised then run that task first then enter the scheduler in this thread
  if (initialTask) {
    initialTask();
  }

  for (;;) {
    if (!running) {
      break;
    }

    auto task = local_policy->getWork();

    if (task) {
      backoff.reset();
      task();
    } else {
      backoff.failed();
      hpx::this_thread::suspend(backoff.getSleepTime());
    }
  }
}

void stopSchedulers() {
  running.store(false);
}

void startSchedulers(unsigned n) {
  hpx::threads::executors::default_executor exe(hpx::threads::thread_priority_critical,
                                                hpx::threads::thread_stacksize_huge);
  for (auto i = 0; i < n; ++i) {
    hpx::apply(exe, &scheduler, nullptr);
  }
}

}}