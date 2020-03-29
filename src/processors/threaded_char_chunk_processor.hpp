#pragma once

#include <cstddef>

#include <future>
#include <utility>
#include <atomic>

#include <boost/utility/string_view.hpp>
#include <boost/lockfree/spsc_queue.hpp>

namespace mtfind
{

template<typename Handler, typename Chunk, size_t kQueueCapacity = 32768>
class ThreadedCharChunkProcessor
{
public:
    explicit ThreadedCharChunkProcessor(Handler handler) : handler_(std::move(handler))
    {}

    ~ThreadedCharChunkProcessor() = default;

    ThreadedCharChunkProcessor(ThreadedCharChunkProcessor const&)            = delete;
    ThreadedCharChunkProcessor& operator=(ThreadedCharChunkProcessor const&) = delete;

    ThreadedCharChunkProcessor(ThreadedCharChunkProcessor&&)            = delete;
    ThreadedCharChunkProcessor& operator=(ThreadedCharChunkProcessor&&) = delete;

    template<typename U = Chunk>
    void operator()(U &&chunk)
    {
        while (!queue_.push(std::forward<U>(chunk)))
            ;
    }

    void start()
    {
        if (!stop_token_)
        {
            worker_ = std::async(std::launch::async, [this]{
                Chunk chunk;

                while (!stop_token_)
                {
                    if (queue_.pop(chunk))
                        handler_(std::move(chunk));
                }

                while (queue_.pop(chunk))
                    handler_(std::move(chunk));
            });
        }
    }

    void stop()
    {
        if (worker_.valid())
        {
            stop_token_ = true;
            worker_.get();
            stop_token_ = false;
        }
    }

private:
    Handler                                                                       handler_;

    std::atomic_bool                                                              stop_token_{false};
    boost::lockfree::spsc_queue<Chunk, boost::lockfree::capacity<kQueueCapacity>> queue_;
    std::future<void>                                                             worker_;
};

} // namespace mtfind
