#ifndef IMAGE_INTEGRATOR_HH
#define IMAGE_INTEGRATOR_HH

#include <string>

#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>

#include <task_pool/task_pool.hh>

const int block_size = 64;

class ImageIntegrator {
public:

    ~ImageIntegrator();
    void test();
    void process(std::string image_path);
    bool try_init(int thread_count);
    void stop();

private:

    struct ImageData {
        ImageData() = default;
        ImageData(std::string path) { try_init(path); }
        
        bool try_init(std::string path);
        void process_block(int block_x, int block_y, int channel);
        std::string block_row_to_string(int y_block_num, int channel);
        
    
        int get_block_id(int x, int y, int channel) {
            return (y * block_count_x + x) * channel_count + channel;
        }
    
        int& get_block_state(int x, int y, int channel) {
            return block_states[get_block_id(x, y, channel)];
        }
    
        int get_id(int x, int y, int channel) {
            return y * image.size[1] * channel_count + 
                x * channel_count + channel;
        }
    
        double& get_res(int x, int y, int channel) {
            return res[get_id(x, y, channel)];
        }
    
        uchar& get_data(int x, int y, int channel) {
            return image.data[get_id(x, y, channel)];
        }
    
        int get_block_row_id(int y, int channel) {
            return channel * block_count_y + y;
        }
    
        std::string& get_block_row_str(int y, int channel) {
            return block_row_str[get_block_row_id(y, channel)];
        }
    
        
        cv::Mat image;
        std::string path;
        std::vector<double> res;
        std::vector<int> block_states;
        std::mutex mtx;
        std::vector<std::string> block_row_str;
        int block_count_x;
        int block_count_y;
        int channel_count;
    };
    
    class TaskWrite : public TaskPool::Task {
    public:
        TaskWrite(
            TaskPool* task_pool, 
            std::shared_ptr<ImageData> image_data,
            int row_block_num,
            int channel
        )
        : Task(task_pool),
        image_data(image_data),
        channel(channel),
        row_block_num(row_block_num) 
        {}
    
        void execute() override;
    
        std::shared_ptr<ImageData> image_data;
        int channel;
        int row_block_num;
    };
    
    class TaskProcess : public TaskPool::Task {
    public:
        TaskProcess(
            TaskPool* task_pool, 
            std::shared_ptr<ImageData> image_data,
            int x_block_start,
            int y_block_start,
            int channel
            )
        : Task(task_pool),
        image_data(image_data),
        x_block_start(x_block_start),
        y_block_start(y_block_start),
        channel(channel) 
        {}
        
        ~TaskProcess() override = default;
    
        void execute() override;
    
        std::shared_ptr<ImageData> image_data;
        int x_block_start;
        int y_block_start;
        int channel;
    };
    
    class TaskRead : public TaskPool::Task {
    public:
        TaskRead(
                TaskPool* task_pool, 
                std::string path, 
                std::shared_ptr<ImageData> image_data) 
        : Task(task_pool),
        path(path),
        image_data(image_data)
        {}
        
        ~TaskRead() override = default;
    
        void execute() override;
    
    
        std::string path;
        std::shared_ptr<ImageData> image_data;
    };
    TaskPool task_pool;
};

#endif
