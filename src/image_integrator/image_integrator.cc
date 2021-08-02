#include <cassert>
#include <chrono>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>

#include <image_integrator/image_integrator.hh>

void ImageIntegrator::test() 
{
    std::cout << "II:test" << std::endl;
}

void ImageIntegrator::process(std::string image_path) 
{
    if (!is_inited) {
        std::cout << "ERROR: ImageIntegrator isn't inited" << std::endl;
    }
    std::shared_ptr<ImageData> image_data_ptr = std::make_shared<ImageData>();
    task_pool.push(new TaskRead{&task_pool, image_path, image_data_ptr});
}

bool ImageIntegrator::try_init(int thread_count) {
    if (thread_count > 0) {
        task_pool.init(thread_count);
    } else if (thread_count == 0) {
        task_pool.init();
    } else {
        std::cout << "ERROR: incorrect thread count" << std::endl;
        return false;
    }

    is_inited = true;
    return true;
}

void ImageIntegrator::stop() {
    is_inited = false;
    task_pool.stop();
}

ImageIntegrator::~ImageIntegrator() {
    task_pool.wait();
}

std::string ImageIntegrator::ImageData::block_row_to_string(
        int y_block_num, 
        int channel
) {
    std::stringstream ss;
    ss.precision(std::numeric_limits<double>::max_digits10);

    const int y_start = y_block_num * default_block_size;
    const int y_end = std::min(image.size[0], (y_block_num + 1) * default_block_size);

    if (image.data != nullptr) {
        for (int y = y_start; y < y_end; y++) {
            for (int x = 0; x < image.size[1]; x++) {
                ss << get_res(x, y, channel) << ' ';
            }
            ss << std::endl;
        }
    }
    return std::move(ss.str());
}
    
bool ImageIntegrator::ImageData::try_init(std::string path) {
    this->path = path;
    image = cv::imread(path, cv::IMREAD_COLOR);

    if (image.data == nullptr) {
        std::cout << "ERROR: image(" << path << ") wasn't found" << std::endl;
        return false;
    }

    const int default_image_dim_count = 2;

    if (image.size.dims() != default_image_dim_count) {
        std::cout << "ERROR: image wasn't found in " << path << std::endl;
        return false;
    }

    block_count_x = round(image.size[1] / double(default_block_size) + 0.5);
    block_count_y = round(image.size[0] / double(default_block_size) + 0.5);
    channel_count = image.channels();

    res.resize(image.size[0] * image.size[1] * channel_count);
    block_row_str.resize(block_count_y * channel_count);
    block_states.resize(channel_count * block_count_x * block_count_y, 0);
    
    for (int i = 0; i < block_count_x; i++) {
        for (int j = 0; j < channel_count; j++) {
            get_block_state(i, 0, j)++;
        }
    }

    for (int i = 0; i < block_count_y; i++) {
        for (int j = 0; j < channel_count; j++) {
            get_block_state(0, i, j)++;
        }
    }

    return true;
}

void ImageIntegrator::ImageData::process_block(
        int block_x, 
        int block_y, 
        int channel
) {
    const int x_start = block_x * default_block_size;
    const int y_start = block_y * default_block_size;
    const int x_end = std::min(image.size[1], (block_x + 1) * default_block_size);
    const int y_end = std::min(image.size[0], (block_y + 1) * default_block_size);

    for (int y = y_start; y != y_end; y++) {
        //accumulator for current row sum
        double acc = 0.0;
        //if column block isn't first we need to correct value
        if (block_x != 0 && y != 0) {
            acc = -get_res(x_start - 1, y - 1, channel);
        }
        for (int x = x_start; x != x_end; x++) {
            double value = 0;
            if (y != 0) {
                //integral image value from upper pixel
                value += get_res(x, y - 1, channel); 
            }

            acc += (int)get_data(x, y, channel);
            //if column block isn't first we need to correct value
            if (x == x_start && block_x != 0) {
                acc += get_res(x_start - 1, y, channel);
            }
            //value only of sum of current row
            value += acc;
            get_res(x, y, channel) = value;
        }
    }
}

void ImageIntegrator::TaskWrite::execute() {
    image_data->get_block_row_str(row_block_num, channel) = 
        std::move(image_data->block_row_to_string(row_block_num, channel));

    {
        std::unique_lock<std::mutex> lock{image_data->mtx};
        int last_blocks_states = 0;

        image_data->get_block_state(
                image_data->block_count_x - 1, 
                row_block_num, 
                channel
        )++;

        for (int y = 0; y < image_data->block_count_y; y++) {
            for (int c = 0; c < image_data->channel_count; c++) {
                last_blocks_states += image_data->get_block_state(
                        image_data->block_count_x - 1, 
                        y, 
                        c
                );
            }
        }

        lock.unlock();
        
        //write to file if all blocks are ready for writing
        if (last_blocks_states == 
                image_data->block_count_y * 3 * image_data->channel_count) {
            const auto filetype = ".integral";
            std::ofstream fout{image_data->path + filetype};
            for (int c = 0; c < image_data->channel_count; c++) {
                for (int i = 0; i < image_data->block_count_y; i++) {
                    fout << image_data->get_block_row_str(i, c);
                }
                fout << std::endl;
            }
            fout.close();
        }
    }
}

void ImageIntegrator::TaskProcess::execute() {
    image_data->process_block(x_block_start, y_block_start, channel);

    auto handle_next_block = [&](int x_block_new, int y_block_new) { 
        if (y_block_new < image_data->block_count_y && 
                x_block_new < image_data->block_count_x) {
            std::unique_lock<std::mutex> lock {image_data->mtx};
            int state_down_block = ++image_data->get_block_state(
                    x_block_new,
                    y_block_new,
                    channel
            );
            lock.unlock();
            if (state_down_block == 2) {
                task_pool->push(new TaskProcess{
                    task_pool,
                    image_data,
                    x_block_new,
                    y_block_new,
                    channel
                });
            }
        }    
        return; 
    };
     
    //change state of next blocks (if it exists)   
    handle_next_block(x_block_start + 1, y_block_start);
    handle_next_block(x_block_start, y_block_start + 1);
    
    //create TaskWrite for last block in row block
    if (x_block_start == image_data->block_count_x - 1) {
        std::unique_lock<std::mutex> lock {image_data->mtx};
            task_pool->push(new TaskWrite{
                task_pool,
                image_data,
                y_block_start,
                channel
            });
    }
}

void ImageIntegrator::TaskRead::execute() {
    if (!image_data->try_init(path)) {
        std::cout << "missing file: " << path << std::endl;
        return;
    } else {
        std::cout << "readed: " << path << std::endl;
    }

    for( int i = 0; i < image_data->channel_count; i++) {
        task_pool->push(new TaskProcess{
            task_pool,
            image_data,
            0,
            0,
            i
        });
    }
}
