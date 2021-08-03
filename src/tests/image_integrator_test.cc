#include <fstream>

#include <gtest/gtest.h>

#include <image_integrator/image_integrator.hh>

TEST(ImageIntegrator, wrong_thread_count) {
    ImageIntegrator ii;
    EXPECT_FALSE(ii.try_init(-1));
}

TEST(ImageIntegrator, init_image_integrator) {
    ImageIntegrator ii;
    EXPECT_TRUE(ii.try_init(0));
}

TEST(ImageIntegrator, uninited_process) {
    ImageIntegrator ii;
    ii.process("somefile");
}

static const int channel_count = 3;

void check_integral_image(std::string path, int mat_size) {
    //integral image of eye matrix 4x4
    //1 1 1 1
    //1 2 2 2
    //1 2 3 3
    //1 2 3 4
    std::ifstream fin{path};
    for (int c = 0; c < channel_count; c++) {
        for (int i = 0; i < mat_size; i++) {
            for (int j = 0; j < mat_size; j++) {
                double readed;
                fin >> readed;
                EXPECT_DOUBLE_EQ(double(1 + std::min(i, j)), readed);
            }
        }
    }
}

TEST(ImageIntegrator, check_integral_image) {
    ImageIntegrator ii;
    EXPECT_TRUE(ii.try_init(0));
    std::string filename = "testfile.tif";
    std::string filetype = ".integral";
    for (int mat_size = 3; mat_size < 16; mat_size+=3 ) {
        cv::Mat M = cv::Mat::eye(mat_size, mat_size, CV_8UC1);
        cv::imwrite(filename, M);
        ii.set_block_size(4);
        ii.process(filename);
        ii.wait();
        check_integral_image(filename + filetype, mat_size);
    }
}

