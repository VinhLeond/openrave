#include <gtest/gtest.h>

#include <openrave/openrave.h>
#include <openrave-core.h>

using namespace std::literals;

class Interface_Test : public ::testing::Test {
public:
    OpenRAVE::EnvironmentBasePtr env;

    // Once per-test
    void SetUp() override {
        env = OpenRAVE::RaveCreateEnvironment("GTestEnvironment");
    }

    // Once per-test
    void TearDown() override {
        env->Destroy();
    }

    // Per suite
    static void SetUpTestSuite() {
        OpenRAVE::RaveInitialize();
        OpenRAVE::RaveSetDebugLevel(OpenRAVE::DebugLevel::Level_Info);
    }

    // Per suite
    static void TearDownTestSuite() {
        OpenRAVE::RaveDestroy();
    }
};

// First environment to be created for this test, ID is 1.
TEST_F(Interface_Test, Environment_Tests)
{
    int envID = OpenRAVE::RaveGetEnvironmentId(env);
    EXPECT_EQ(envID, 1);
    auto ptr = OpenRAVE::RaveGetEnvironment(envID);
    EXPECT_EQ(ptr->GetName(), env->GetName());
}

TEST_F(Interface_Test, Print_Modules)
{
    std::list<OpenRAVE::ModuleBasePtr> modules;
    env->GetModules(modules);
    for (OpenRAVE::ModuleBasePtr& module : modules) {
        std::cout << module->GetPluginName() << std::endl;
    }
}

TEST_F(Interface_Test, Print_Plugins)
{
    std::list<std::pair<std::string, OpenRAVE::PLUGININFO>> plugins;
    OpenRAVE::RaveGetPluginInfo(plugins);
    for (const auto& pair : plugins) {
        std::cout << pair.first << std::endl;
    }
}

TEST_F(Interface_Test, Print_Interfaces)
{
    std::map<OpenRAVE::InterfaceType, std::vector<std::string>> interfaces;
    OpenRAVE::RaveGetLoadedInterfaces(interfaces);
    for (const auto& pair : interfaces) {
        std::cout << OpenRAVE::RaveGetInterfaceName(pair.first) << std::endl;
        for (const std::string& interface : pair.second) {
            std::cout << '\t' << interface << std::endl;
        }
    }
}

TEST_F(Interface_Test, Create_KinBody_Robot)
{
    OpenRAVE::KinBodyPtr body = OpenRAVE::RaveCreateKinBody(env, "Interface_Test_Create_KinBody");
    EXPECT_TRUE(body);
    OpenRAVE::RobotBasePtr robot = OpenRAVE::RaveCreateRobot(env, "Interface_Test_Create_Robot");
    EXPECT_TRUE(robot);
}
