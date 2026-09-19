#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Person/Body/Body.hpp"
#include "Singularity/Storage/SaveSystem.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <filesystem>

namespace {

struct TestEnvironment {
    std::filesystem::path tempDir;
    std::string prevSaveRoot;

    TestEnvironment() {
        tempDir = std::filesystem::temp_directory_path() / "earthcall_person_test";
        std::filesystem::remove_all(tempDir);
        std::filesystem::create_directories(tempDir);

        prevSaveRoot = SaveSystem::saveRoot();
        SaveSystem::setSaveRoot(tempDir.string());
    }

    ~TestEnvironment() {
        SaveSystem::setSaveRoot(prevSaveRoot);
        std::filesystem::remove_all(tempDir);
    }
};


bool near(float a, float b) {
    return std::fabs(a - b) < 0.0001f;
}

void test_initialization() {
    Soul soul("TestSoul");
    Body body("Humanoid", "Voxel");
    Person p(std::move(soul), std::move(body), "default");

    // "Player" will be resolved to "Person" inside setDisplayName via seedName.
    // wait, what is the default name after init?
    // Let's assert what is true.
    assert(p.soul().getIdentifier() == "TestSoul");
}

void test_position_and_velocity() {
    Person p(Soul("T"), Body("Humanoid", "Voxel"), "default");

    p.position() = {1.0f, 2.0f, 3.0f};
    assert(near(p.position().x, 1.0f));
    assert(near(p.position().y, 2.0f));
    assert(near(p.position().z, 3.0f));

    p.velocity() = {4.0f, 5.0f, 6.0f};
    assert(near(p.velocity().x, 4.0f));
    assert(near(p.velocity().y, 5.0f));
    assert(near(p.velocity().z, 6.0f));
}

void test_naming() {
    Person p(Soul("T"), Body("Humanoid", "Voxel"), "default");

    p.setDisplayName("Alice");
    assert(p.getDisplayName() == "Alice");

    p.rename("Bob");
    assert(p.getDisplayName() == "Bob");

    // Edge cases for empty or "Player"
    p.rename("");
    assert(p.getDisplayName() == "Person");

    p.rename("player");
    assert(p.getDisplayName() == "Person");
}

void test_session_state() {
    Person p(Soul("T"), Body("Humanoid", "Voxel"), "default");

    assert(!p.isLoggedIn());
    assert(p.getCurrentSession().empty());

    p.login("session123");
    assert(p.isLoggedIn());
    assert(p.getCurrentSession() == "session123");

    // calling login again shouldn't change session if already logged in (based on impl)
    p.login("session456");
    assert(p.isLoggedIn());
    assert(p.getCurrentSession() == "session123");

    p.logout();
    assert(!p.isLoggedIn());
    assert(p.getCurrentSession().empty());

    // login without param generates session
    p.login();
    assert(p.isLoggedIn());
    assert(!p.getCurrentSession().empty());
}

void test_body_management() {
    Person p(Soul("T"), Body("Humanoid", "Voxel"), "default");

    // initially 1 body
    assert(p.getBody().shape == "Humanoid");

    Body b2("Quadruped", "Voxel");
    p.addBody(std::move(b2));

    p.setActiveBody(1);
    assert(p.getBody().shape == "Quadruped");

    // out of bounds index shouldn't change
    p.setActiveBody(999);
    assert(p.getBody().shape == "Quadruped");
}

} // namespace

int main() {
    TestEnvironment env;

    test_initialization();
    test_position_and_velocity();
    test_naming();
    test_session_state();
    test_body_management();

    std::puts("person_test: ALL OK");
    return 0;
}
