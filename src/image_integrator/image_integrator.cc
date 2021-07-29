#include <iostream>

#include <image_integrator/image_integrator.hh>

void ImageIntegrator::test() 
{
    std::cout << "II:test" << std::endl;
}

void ImageIntegrator::process(std::string image_path) 
{
    std::cout << "II: process : " << image_path << std::endl;
}
