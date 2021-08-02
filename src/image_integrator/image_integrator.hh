#ifndef IMAGE_INTEGRATOR_HH
#define IMAGE_INTEGRATOR_HH

#include <string>

#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>

#include <multithread_utils/thread_pool.hh>

/// Class for creating integral images
/**
 * It creates integral images along given paths to the original image 
 */
class ImageIntegrator {
public:

    ImageIntegrator() = default;
    ImageIntegrator(ImageIntegrator&) = delete;
    ImageIntegrator& operator= (const ImageIntegrator& ) = delete;
    ~ImageIntegrator();
    /// Create an integral image
    /**
     *  Save multichannel integral image to new file *original_name*.integral in 
     *  following format:
     *  0.0 1.0
     *  2.0 6.0
     *  6.0 15.0
     *  each channels divided by empty line
     *
     *  \param[in] image_path Path to image
     */
    void process(std::string image_path);
    /** 
     *  Try to initialize with the given number of threads. If the number of 
     *  threads is specified incorrectly, then it will return false
     *  \param[in] thread_count Count of threads for thread pool
     */
    bool try_init(int thread_count);
    /// Wait all tasks to finish and stop integrator
    void stop();
    /// Wait all tasks to finish 
    void wait();
    ///set block size for parallel processing, default is 64
    void set_block_size(int block_size) { this->block_size = block_size; }
    ///get block size for parallel processing, default is 64
    int get_block_size() { return block_size; }

private:

    /// Structure for storing all the information necessary for processing 
    struct ImageData {
        ImageData() = default;
        ImageData(ImageData&) = default;
        ImageData& operator= (const ImageData&) = default;
        ImageData(int block_size)
        :block_size(block_size)
        {}
        ImageData(std::string path) { try_init(path); }
        
        /// try to read image and create all processing structs if succesed
        bool try_init(std::string path);
        /// process block of image
        void process_block(
                int block_x, 
                int block_y, 
                int channel
        );
        /// create string of all blocks in row for writing it to file
        std::string block_row_to_string(int y_block_num, int channel) const;
        
        int get_block_id(int x, int y, int channel) const {
            return (y * block_count_x + x) * channel_count + channel;
        }
    
        int8_t& get_block_state(int x, int y, int channel) {
            return block_states[get_block_id(x, y, channel)];
        }

        int8_t get_block_state(int x, int y, int channel) const {
            return block_states[get_block_id(x, y, channel)];
        }
    
        int get_id(int x, int y, int channel) const {
            return y * image.size[1] * channel_count + 
                x * channel_count + channel;
        }
    
        double& get_res(int x, int y, int channel) {
            return res[get_id(x, y, channel)];
        }

        double get_res(int x, int y, int channel) const {
            return res[get_id(x, y, channel)];
        }
    
        uchar& get_data(int x, int y, int channel) {
            return image.data[get_id(x, y, channel)];
        }

        uchar get_data(int x, int y, int channel) const {
            return image.data[get_id(x, y, channel)];
        }
    
        int get_block_row_id(int y, int channel) const {
            return channel * block_count_y + y;
        }
    
        std::string& get_block_row_str(int y, int channel) {
            return block_row_str[get_block_row_id(y, channel)];
        }

        std::string get_block_row_str(int y, int channel) const {
            return block_row_str[get_block_row_id(y, channel)];
        }
    
        /// readed from OpenCV image
        cv::Mat image;
        /// path of image
        std::string path;
        /// data for integral image
        std::vector<double> res;
        /// size of block
        int block_size = 64;
        /// states of all blocks
        /**
         *  Data is splitted in several blocks with different states:
         *  
         *  0 - Block waits for both previous(up, left) blocks to be processed 
         *  1 - Block waits for one more previous blocks to be processed 
         *  2 - Block is ready for processing
         *  3 - (Only for last block in block row) Row block string is ready
         *
         *  Each channel has its own block. 
         */
        std::vector<int8_t> block_states;
        /// for work with block_states
        std::mutex mtx;
        /// strings for writing to file
        std::vector<std::string> block_row_str;
        /// count of block in x axis
        int block_count_x;
        /// count of block in y axis
        int block_count_y;
        /// count of image channels
        int channel_count;
    };
    
    /**
     *  Task for creating string of integrated image's block and writing it to 
     *  file, when all row block strings is ready
     */
    class TaskWrite : public ThreadPool::Task {
    public:
        TaskWrite(
            ThreadPool* task_pool, 
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
    
    /**
     * Processes the specified block and starts other TaskProcess if right or 
     * down block has state 2. Start TaskWrite if current block is last in row.
     */
    class TaskProcess : public ThreadPool::Task {
    public:
        TaskProcess(
            ThreadPool* task_pool, 
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
    
    /**
     *  Read image and creates all neccassary data structs. Create TaskProcess 
     *  for all channels with block (0,0)
     */
    class TaskRead : public ThreadPool::Task {
    public:
        TaskRead(
                ThreadPool* task_pool, 
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

    ThreadPool task_pool;
    int block_size = 64;
    bool is_inited = false;
};

#endif
