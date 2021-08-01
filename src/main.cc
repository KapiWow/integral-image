#include <cxxopts.hpp>

#include <iostream>

#include <image_integrator/image_integrator.hh>

int main(int argc, char** argv) 
{
    cxxopts::Options options("integrate_image", "create integral images");

    options.add_options()
        ("t,threads", "thread count", cxxopts::value<int>()->default_value("0"))
        ("i,image", "input image", cxxopts::value<std::vector<std::string>>())
    ;

    auto parse_result = options.parse(argc, argv);

    if (parse_result.count("help"))
    {
      std::cout << options.help() << std::endl;
      return 0;
    }
    
    int thread_count = parse_result["threads"].as<int>();

    ImageIntegrator ii;
    if (!ii.try_init(parse_result["threads"].as<int>())) {
        return 0;
    }

    if (parse_result.count("image")) {
        auto& vec = parse_result["image"].as<std::vector<std::string>>();
        std::for_each(vec.begin(), vec.end(), 
                [&] (std::string value) { ii.process(value); });
    }
    return 0;
}
