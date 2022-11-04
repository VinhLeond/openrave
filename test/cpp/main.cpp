#include <errno.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <boost/program_options.hpp>
#include <gtest/gtest.h>

#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>

// This value should be predefined by CMake.
#ifndef OPENRAVE_DEFAULT_PLUGIN_PATH
#define OPENRAVE_DEFAULT_PLUGIN_PATH "/opt/lib/openrave0.100-plugins/"
#endif // OPENRAVE_DEFAULT_PLUGIN_PATH

rapidjson::Document SCENE;

bool ReadInScene(const std::string& scene) {
    auto ifs = std::ifstream(scene, std::ios::ate);
    if (!ifs) {
        //std::cerr << "ERROR! Scene file at " << scene << " does not exist. File is required to run tests." << std::endl;
        return true;
    } else {
        std::cout << "Reading in scene file at " << scene << std::endl;
    }
    size_t size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    std::string file;
    file.resize(size, '\0');
    ifs.read(&file[0], size);

    SCENE.Parse(file);
    return true;
}

// Ensure that plugins can be found and loaded.
class OpenRaveGlobalTestEnvironment final : public ::testing::Environment {
public:
    static constexpr const char* OPENRAVE_PLUGINS = "OPENRAVE_PLUGINS";

    ~OpenRaveGlobalTestEnvironment() override {}

    void SetUp() override {
        char* plugin_path = ::getenv(OPENRAVE_PLUGINS);
        if (plugin_path) {
            std::cout << "Using plugins from path " << plugin_path << std::endl;
            return;
        }
        int res = ::setenv(OPENRAVE_PLUGINS, OPENRAVE_DEFAULT_PLUGIN_PATH, 0);
        if (res == -1) {
            std::cerr << "Failed to setenv OPENRAVE_PLUGINS; " << ::strerror(errno) << std::endl;
            return;
        }
        plugin_path = ::getenv(OPENRAVE_PLUGINS);
        std::cout << "Using plugins from path " << plugin_path << std::endl;
    }

    void TearDown() override {}
};

int main(int argc, char* argv[]) {
    SCENE = rapidjson::Document();
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new OpenRaveGlobalTestEnvironment());

    std::string scene; // Path to a scene file. We assume, with great naivety, that it is valid.
    namespace po = boost::program_options;
    po::options_description desc;
    desc.add_options()
        ("scene", po::value<std::string>(&scene), "Path to a scene file.")
        ("help", "Prints this help message.")
        ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    if (vm.count("help") > 0) {
        std::cout << desc << std::endl;
        return EXIT_FAILURE;
    }
    if (!ReadInScene(scene)) {
        return EXIT_FAILURE;
    }

    return RUN_ALL_TESTS();
}
