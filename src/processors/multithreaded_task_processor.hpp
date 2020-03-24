#pragma once

#include <cstddef>

#include <thread>
#include <vector>
#include <future>
#include <utility>
#include <memory>

#include <boost/range/adaptor/filtered.hpp>
#include <boost/utility/string_view.hpp>
#include <boost/asio/io_service.hpp>

namespace mtfind
{

class MultithreadedTaskProcessor
{
public:
    explicit MultithreadedTaskProcessor(size_t workers = std::thread::hardware_concurrency())
        : workers_(workers > 0 ? workers : 1)
    {}

    ~MultithreadedTaskProcessor() = default;

    MultithreadedTaskProcessor(MultithreadedTaskProcessor const&)            = delete;
    MultithreadedTaskProcessor& operator=(MultithreadedTaskProcessor const&) = delete;

    MultithreadedTaskProcessor(MultithreadedTaskProcessor&&)            = delete;
    MultithreadedTaskProcessor& operator=(MultithreadedTaskProcessor&&) = delete;

    template<typename Task>
    void operator()(Task task) { io_.post(task); }

    void run()
    {
        if (!work_)
        {
            io_.reset();
            work_ = std::make_unique<boost::asio::io_service::work>(io_);
            for (auto &worker : workers_)
                worker = std::async(std::launch::async, [this]{ io_.run(); });
        }
    }

    void wait()
    {
        work_.reset();
        auto valid_workers = workers_ | boost::adaptors::filtered([](auto const &worker){ return worker.valid(); });
        for (auto &worker : valid_workers)
            worker.get();
    }

    void stop() { io_.stop(); }


    auto workers_count() const noexcept { return workers_.size(); }

private:
    boost::asio::io_service                        io_;
    std::vector<std::future<void>>                 workers_;
    std::unique_ptr<boost::asio::io_service::work> work_;
};

} // namespace mtfind
