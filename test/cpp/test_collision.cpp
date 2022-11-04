#include <gtest/gtest.h>

#include <openrave/openrave.h>
#include <openrave-core.h>
#include <rapidjson/document.h>

using namespace std::literals;

extern rapidjson::Document SCENE; // defined in main.cpp

// Tests involving collision checkers.
// Although all tests nominally should involve all supported collision checkers,
// some implementations are incomplete. They have been marked with 'TEST_WORKAROUND' in the tests.

class Collision_Test : public ::testing::TestWithParam<std::string> {
public:
    OpenRAVE::EnvironmentBasePtr env;
    OpenRAVE::CollisionCheckerBasePtr collisionChecker;

    // Once per-test
    void SetUp() override {
        env = OpenRAVE::RaveCreateEnvironment("GTestEnvironment");
        if (SCENE.Empty()) {
            GTEST_SKIP() << "Scene is empty, skipping. Pass in a valid scene file.";
        }
        std::vector<OpenRAVE::KinBodyPtr> created, modified, removed;
        env->LoadJSON(SCENE, OpenRAVE::UpdateFromInfoMode::UFIM_Exact, created, modified, removed);
        collisionChecker = OpenRAVE::RaveCreateCollisionChecker(env, GetParam());
    }

    // Once per-test
    void TearDown() override {
        collisionChecker.reset();
        env->Destroy();
    }

    // Per suite
    static void SetUpTestSuite() {
        OpenRAVE::RaveInitialize();
        OpenRAVE::RaveSetDebugLevel(OpenRAVE::DebugLevel::Level_Error);
    }

    // Per suite
    static void TearDownTestSuite() {
        OpenRAVE::RaveDestroy();
    }
};

// Places a single AABB box into an empty environment, and asserts that it collides with nothing.
TEST_P(Collision_Test, AABB_No_Collision)
{
    auto position = OpenRAVE::Vector(0, 0, 0);
    auto extent = OpenRAVE::Vector(1, 1, 1);
    auto pose = OpenRAVE::Transform();
    pose.identity();
    auto aabb = OpenRAVE::AABB(position, extent);

    if (GetParam() == "pqp" || GetParam() == "ode") {
        GTEST_SKIP() << "TEST_WORKAROUND: Not implemented in PQP and ODE.";
    }
    auto report = boost::make_shared<OpenRAVE::CollisionReport>();
    EXPECT_FALSE(collisionChecker->CheckCollision(aabb, pose, report));
    EXPECT_TRUE(report->contacts.empty());
}

// Initialized KinBodies
TEST_P(Collision_Test, Robot_KinBody_Collision)
{
    auto robot = env->GetRobot("motoman-gp70l");
    auto obstacle = env->GetKinBody("BigBoyCollision");
    collisionChecker->InitKinBody(robot);
    collisionChecker->InitKinBody(obstacle);
    auto report = boost::make_shared<OpenRAVE::CollisionReport>();

    if (GetParam() == "pqp") {
        GTEST_SKIP() << "TEST_WORKAROUND: Broken in PQP.";
    }
    EXPECT_TRUE(collisionChecker->CheckCollision(robot, obstacle, report));
    EXPECT_TRUE(report->plink1);
}

// Uninitialized KinBodies should have no effect on collision.
TEST_P(Collision_Test, Robot_KinBody_Collision_NoInit)
{
    auto robot = env->GetRobot("motoman-gp70l");
    auto obstacle = env->GetKinBody("BigBoyCollision");
    //collisionChecker->InitKinBody(robot);     // Do not remove this line
    //collisionChecker->InitKinBody(obstacle);  // Do not remove this line
    auto report = boost::make_shared<OpenRAVE::CollisionReport>();

    if (GetParam() == "ode") {
        EXPECT_TRUE(collisionChecker->CheckCollision(robot, obstacle, report))
            << "TEST_WORKAROUND: ODE can collide uninitialized KinBodies.";
    } else {
        EXPECT_FALSE(collisionChecker->CheckCollision(robot, obstacle, report));
    }
}

// Robot should not be in collision with itself.
TEST_P(Collision_Test, Robot_Self_Collision)
{
    auto robot = env->GetRobot("motoman-gp70l");
    collisionChecker->InitKinBody(robot);

    auto report = boost::make_shared<OpenRAVE::CollisionReport>();
    EXPECT_FALSE(collisionChecker->CheckStandaloneSelfCollision(robot, report));
}

// A single link should not collide with itself.
TEST_P(Collision_Test, Robot_Self_Collision2)
{
    auto robot = env->GetRobot("motoman-gp70l");
    collisionChecker->InitKinBody(robot);

    auto links = robot->GetLinks();
    auto report = boost::make_shared<OpenRAVE::CollisionReport>();
    for (const auto& link : links) {
        EXPECT_FALSE(collisionChecker->CheckStandaloneSelfCollision(link, report));
        EXPECT_TRUE(report->contacts.empty());
    }
}

// Disabled ode and bullet collision for now because they don't work
INSTANTIATE_TEST_SUITE_P(CollisionTest, Collision_Test, ::testing::Values(
    "fcl_"s,
    "ode"s,
    //"bullet"s, // TEST_WORKAROUND: bullet is broken, gives segfault
    "pqp"s
));