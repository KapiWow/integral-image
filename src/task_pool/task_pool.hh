#ifndef TASK_POOL_HH
#define TASK_POOL_HH

#include <atomic>
#include <vector>
#include <thread>
#include <condition_variable>
#include <queue>
#include <chrono>

class TaskPool
{
public:
    
    TaskPool() = default;

    ~TaskPool() {
        stop();
    }

    class Task {
        public:
        Task(TaskPool* task_pool) {
            this->task_pool = task_pool;
        }

        virtual ~Task() = default;

        virtual void execute() = 0;
        
        TaskPool *task_pool;
    };

    void init (int thread_count);
    void init () { init(std::thread::hardware_concurrency()); }
    void push(Task* task);

    void wait();
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
