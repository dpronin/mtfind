#pragma once

#include <cstddef>

#include <future>
#include <utility>
#include <atomic>

#include <boost/lockfree/spsc_queue.hpp>

namespace mtfind
{

///
/// @brief      This class describes a threaded chunk processor
///             It receives chunks from one thread and handles them in the other with a handler
///
/// @tparam     Handler         A handler type
/// @tparam     Chunk           A chunk type
/// @tparam     kQueueCapacity  Capacity of the internal queue that is used to pass chunks from
///                             one thread to the other
///
template<typename Handler, typename Chunk, size_t kQueueCapacity = 32768>
class ThreadedChunkProcessor
{
public:
    ///
    /// @brief      Constructs the processor
    ///
    /// @param[in]  handler  The handler that will be called in
    ///                      the other concurrent thread-receiver
    ///
    explicit ThreadedChunkProcessor(Handler handler) : handler_(std::move(handler))
    {}

    ~ThreadedChunkProcessor() = default;

    ThreadedChunkProcessor(ThreadedChunkProcessor const&)            = delete;
    ThreadedChunkProcessor& operator=(ThreadedChunkProcessor const&) = delete;

    ThreadedChunkProcessor(ThreadedChunkProcessor&&)            = delete;
    ThreadedChunkProcessor& operator=(ThreadedChunkProcessor&&) = delete;

    ///
    /// @brief      Receives the chunk to process with the handler
    ///             in the other thread
    ///
    /// @param      chunk  The chunk to push to the queue
    ///
    /// @tparam     U any type convertable to Chunk or Chunk itself
    ///
    template<typename U = Chunk>
    bool operator()(U &&chunk)
    {
        return queue_.push(std::forward<U>(chunk));
    }

    ///
    /// @brief      Starts the receiver to process chunks by popping from
    ///             the lockless queue without blocking
    ///
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

    ///
    /// @brief      Stops the receiver and waits until it handles off
    ///             all the chunks left in the queue
    ///
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
