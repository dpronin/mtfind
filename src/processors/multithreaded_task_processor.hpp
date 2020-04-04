#pragma once

#include <cstddef>

#include <future>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

#include <boost/asio/io_service.hpp>
#include <boost/range/adaptor/filtered.hpp>

namespace mtfind
{

///
/// @brief      This class describes a multithreaded task processor.
///             It is dedicated to concurrently running the tasks in several threads
///
class MultithreadedTaskProcessor
{
public:
    explicit MultithreadedTaskProcessor(size_t workers = std::thread::hardware_concurrency())
        : workers_(workers > 0 ? workers : 1)
    {
    }

    ~MultithreadedTaskProcessor() = default;

    MultithreadedTaskProcessor(MultithreadedTaskProcessor const &) = delete;
    MultithreadedTaskProcessor &operator=(MultithreadedTaskProcessor const &) = delete;

    MultithreadedTaskProcessor(MultithreadedTaskProcessor &&) = delete;
    MultithreadedTaskProcessor &operator=(MultithreadedTaskProcessor &&) = delete;

    ///
    /// @brief      Schedule a task to the processor
    ///
    /// @param      task  The task
    ///
    /// @tparam     Task
    ///
    template <typename Task>
    void operator()(Task &&task) { io_.post(std::forward<Task>(task)); }

    ///
    /// @brief      Runs the processor so that it can schedule and run tasks
    ///             passed into operator()
    ///
    void run()
    {
        if (!work_)
        {
            io_.reset();
            work_ = std::make_unique<boost::asio::io_service::work>(io_);
            for (auto &worker : workers_)
                worker = std::async(std::launch::async, [this] { io_.run(); });
        }
    }

    ///
    /// @brief      Waits until all the tasks are done
    ///
    void wait()
    {
        work_.reset();
        auto valid_workers = workers_ | boost::adaptors::filtered([](auto const &worker) { return worker.valid(); });
        for (auto &worker : valid_workers)
            worker.get();
    }

    ///
    /// @brief      Stops the processor as soon as possible
    ///
    /// @details    All tasks that have been pushed but not scheduled yet
    ///             are cancelled and not be executed
    ///
    void stop() { io_.stop(); }

    ///
    /// @brief      Gets a number of threads used
    ///
    /// @return     A number of threads
    ///
    auto workers_count() const noexcept { return workers_.size(); }

private:
    boost::asio::io_service                        io_;
    std::vector<std::future<void>>                 workers_;
    std::unique_ptr<boost::asio::io_service::work> work_;
};

} // namespace mtfind
