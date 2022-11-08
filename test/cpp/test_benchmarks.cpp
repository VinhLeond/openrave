#include <chrono>
#include <gtest/gtest.h>

#include <openrave/openrave.h>
#include <openrave-core.h>

class Benchmark : public ::testing::Test {
public:
    // Per suite
    void SetUp() override {
        OpenRAVE::RaveInitialize();
        OpenRAVE::RaveSetDebugLevel(OpenRAVE::DebugLevel::Level_Info);
        start = std::chrono::high_resolution_clock::now();
    }

    // Per suite
    void TearDown() override {
        auto delta = std::chrono::high_resolution_clock::now() - start;
        std::cout << "The above test took " << delta.count() << " nanoseconds." << std::endl;
        OpenRAVE::RaveDestroy();
    }

private:
    std::chrono::system_clock::time_point start;
};

TEST_F(Benchmark, Environment_Creation)
{   
    OpenRAVE::EnvironmentBasePtr newEnv = OpenRAVE::CreateEnvironment(true);
    std::list<std::pair<std::string, OpenRAVE::PLUGININFO>> plugins;
    OpenRAVE::RaveGetPluginInfo(plugins);
}