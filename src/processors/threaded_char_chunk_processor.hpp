#pragma once

#include <cstddef>

#include <future>
#include <utility>
#include <atomic>

#include <boost/utility/string_view.hpp>
#include <boost/lockfree/spsc_queue.hpp>

namespace mtfind
{

template<typename Handler>
class ThreadedCharChunkProcessor
{
public:
    explicit ThreadedCharChunkProcessor(Handler chunk_handler) : chunk_handler_(std::move(chunk_handler))
    {}

    ~ThreadedCharChunkProcessor() = default;

    ThreadedCharChunkProcessor(ThreadedCharChunkProcessor const&)            = delete;
    ThreadedCharChunkProcessor& operator=(ThreadedCharChunkProcessor const&) = delete;

    ThreadedCharChunkProcessor(ThreadedCharChunkProcessor&&)            = delete;
    ThreadedCharChunkProcessor& operator=(ThreadedCharChunkProcessor&&) = delete;

    void operator()(size_t chunk_idx, boost::string_view chunk_view)
    {
        while (!queue_.push({chunk_idx, chunk_view}))
            ;
    }

    void start()
    {
        if (!stop_token_)
        {
            worker_ = std::async(std::launch::async, [this]{
                typename decltype(queue_)::value_type chunk;

                while (!stop_token_)
                {
                    if (queue_.pop(chunk))
                        chunk_handler_(chunk.idx, chunk.view);
                }

                while (queue_.pop(chunk))
                    chunk_handler_(chunk.idx, chunk.view);
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
    struct Chunk
    {
        size_t             idx;
        boost::string_view view;
    };

private:
    static constexpr size_t kQCapacity = 32768;

    Handler                                                                   chunk_handler_;

    std::atomic_bool                                                          stop_token_{false};
    boost::lockfree::spsc_queue<Chunk, boost::lockfree::capacity<kQCapacity>> queue_;
    std::future<void>                                                         worker_;
};

} // namespace mtfind
