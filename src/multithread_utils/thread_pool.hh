#ifndef TASK_POOL_HH
#define TASK_POOL_HH

#include <atomic>
#include <vector>
#include <thread>
#include <condition_variable>
#include <queue>
#include <chrono>

///brief Simple thread pool implementation
/**
 *  It allows to create your own tasks and add it to the queue. These tasks will 
 *  be executed in parallel.
 */
class ThreadPool
{
public:
    
    ThreadPool() = default;

    ~ThreadPool() {
        stop();
    }

    ///brief Base class for creating your own tasks    
    class Task {
        public:
        Task(ThreadPool* task_pool) {
            this->task_pool = task_pool;
        }

        virtual ~Task() = default;

         ///The function that will be executed when the task is removed 
         //from the queue 
        virtual void execute() = 0;
        
        ThreadPool *task_pool;
    };

    /**
     *  Init thread pool with a given number of threads 
     *  \param[in] thread_count Count of threads in pool
     */
    void init (int thread_count);
    /**
     *  Init thread pool with a number of threads based on your system 
     */
    void init () { init(std::thread::hardware_concurrency()); }
    /// Push task to queue
    /**
     * Task will be deleted after execution
     */
    void push(Task* task);

    /// Wait for all running tasks to finish 
    void wait();
    /// Finish all tasks and clear pool
    void stop();

private:
    void task_loop();

    std::condition_variable cv;
    std::queue<Task*> tasks;
    std::vector<std::thread> pool;
    std::mutex mtx;
	std::atomic<bool> stopped{false};
    std::atomic<int> num_thread_alive{0};
    std::atomic<int> num_task_executing{0};

};

#endif
