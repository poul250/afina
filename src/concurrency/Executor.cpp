// #include <afina/concurrency/Executor.h>
#include "/home/pavel/prog/afina/include/afina/concurrency/Executor.h"

namespace Afina {
namespace Concurrency {

void perform(Executor * executor)
{
    while (true) {
        std::function<void()> task = nullptr;

        {
            std::unique_lock<std::mutex> lock(executor->mutex);
            std::cv_status status = std::cv_status::no_timeout;
            auto stop_time = std::chrono::steady_clock::now() + executor->idle_time;

            while (std::chrono::steady_clock::now() < stop_time
                && executor->state == Executor::State::kRun
                && executor->tasks.empty())
            {
                status = executor->empty_condition.wait_until(lock, stop_time);
            }

            if (status == std::cv_status::timeout
                && executor->threads.size() > executor->low_watermark)
            {
                break;
            } else if (executor->state != Executor::State::kRun) {

            } else if (executor->state == Executor::State::kRun) {
                task = executor->tasks.front();
                executor->tasks.pop_front();
            } else {
                break;
            }
        }

        task();
    }
}

Executor::Executor(size_t low_watermark,
         size_t high_watermark,
         size_t max_queue_size,
         std::chrono::milliseconds)
    : state(State::kRun)
    , low_watermark(low_watermark)
    , n_threads(low_watermark)
    , high_watermark(std::max(low_watermark, high_watermark))
    , max_queue_size(max_queue_size)
{
    threads.reserve(low_watermark);
    for (int i = 0; i < low_watermark; ++i) {
        threads.push_back(std::thread(perform, this));
    }
}

Executor::~Executor()
{
}

void Executor::Stop(bool await)
{

}

} // namespace Concurrency
} // namespace Afina
