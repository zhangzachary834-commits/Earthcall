#include "Person/Perspective/PerspectiveManager.hpp"
#include "Person/Perspective/PersonPerspective.hpp"
#include <cassert>
#include <cstdio>
#include <iostream>

namespace {

void test_initialization() {
    PerspectiveManager manager;
    assert(manager.count() == 0);
    assert(manager.currentIndex() == 0);
    assert(!manager.isActive());
    assert(manager.current() == nullptr);
}

void test_add_perspective() {
    PerspectiveManager manager;
    manager.addPerspective(std::make_unique<PersonPerspective>("P1", PersonPerspective::PerspectiveType::FirstPerson));

    assert(manager.count() == 1);
    assert(manager.currentIndex() == 0);
    assert(manager.current() != nullptr);
    assert(manager.current()->getName() == "P1");

    // Add another perspective
    manager.addPerspective(std::make_unique<PersonPerspective>("P2", PersonPerspective::PerspectiveType::ThirdPerson));

    assert(manager.count() == 2);
    assert(manager.currentIndex() == 0); // Still 0
    assert(manager.hasPerspective("P2"));

    // Test adding duplicate name
    manager.addPerspective(std::make_unique<PersonPerspective>("P1", PersonPerspective::PerspectiveType::FreeCamera));
    assert(manager.count() == 2); // Should not add duplicate
}

void test_switch_perspective() {
    PerspectiveManager manager;
    manager.addPerspective(std::make_unique<PersonPerspective>("P1", PersonPerspective::PerspectiveType::FirstPerson));
    manager.addPerspective(std::make_unique<PersonPerspective>("P2", PersonPerspective::PerspectiveType::ThirdPerson));
    manager.addPerspective(std::make_unique<PersonPerspective>("P3", PersonPerspective::PerspectiveType::TopDown));

    assert(manager.currentIndex() == 0);

    // Switch by index
    manager.switchTo(1);
    assert(manager.currentIndex() == 1);
    assert(manager.current()->getName() == "P2");

    // Switch by name
    manager.switchTo("P3");
    assert(manager.currentIndex() == 2);
    assert(manager.current()->getName() == "P3");

    // Switch to invalid index (should not change)
    manager.switchTo(999);
    assert(manager.currentIndex() == 2);

    // Switch to invalid name (should not change)
    manager.switchTo("Invalid");
    assert(manager.currentIndex() == 2);
}

void test_remove_perspective() {
    PerspectiveManager manager;
    manager.addPerspective(std::make_unique<PersonPerspective>("P1"));
    manager.addPerspective(std::make_unique<PersonPerspective>("P2"));
    manager.addPerspective(std::make_unique<PersonPerspective>("P3"));

    manager.switchTo(1); // Set current to P2

    // Remove non-current perspective
    manager.removePerspective(2); // Remove P3
    assert(manager.count() == 2);
    assert(manager.currentIndex() == 1); // Still points to P2

    // Remove current perspective
    manager.removePerspective(1); // Remove P2
    assert(manager.count() == 1);
    assert(manager.currentIndex() == 0); // Adjusted to P1
    assert(manager.current()->getName() == "P1");

    // Remove last perspective
    manager.removePerspective(0);
    assert(manager.count() == 0);
    assert(manager.currentIndex() == 0);
    assert(manager.current() == nullptr);

    // Remove invalid index
    manager.removePerspective(999);
    assert(manager.count() == 0);
}

void test_state_management() {
    PerspectiveManager manager;
    manager.addPerspective(std::make_unique<PersonPerspective>("P1"));
    manager.addPerspective(std::make_unique<PersonPerspective>("P2"));

    assert(!manager.isActive());
    assert(!manager.current()->isActive());

    manager.activate();
    assert(manager.isActive());
    assert(manager.current()->isActive());

    // Switching while active should activate the new perspective and deactivate the old
    manager.switchTo(1);
    assert(manager.isActive());
    assert(manager.current()->isActive()); // P2 is active
    assert(!manager.get(0)->isActive());   // P1 is inactive

    manager.deactivate();
    assert(!manager.isActive());
    assert(!manager.current()->isActive()); // P2 is inactive
}

void test_clear() {
    PerspectiveManager manager;
    manager.addPerspective(std::make_unique<PersonPerspective>("P1"));
    manager.addPerspective(std::make_unique<PersonPerspective>("P2"));

    manager.activate();
    assert(manager.isActive());

    manager.clear();
    assert(manager.count() == 0);
    assert(manager.currentIndex() == 0);
    assert(manager.current() == nullptr);
    // isActive state of manager is kept (depends on implementation, actually clears only perspectives)
    assert(manager.isActive());
}

void test_distinct_perspectives_same_spelling() {
    PerspectiveManager manager;
    // Two distinct perspectives that share the same display name ("Overhead View")
    manager.addPerspective(std::make_unique<PersonPerspective>("p1_cam", "Overhead View", PersonPerspective::PerspectiveType::TopDown));
    manager.addPerspective(std::make_unique<PersonPerspective>("p2_cam", "Overhead View", PersonPerspective::PerspectiveType::Isometric));

    assert(manager.count() == 2);
    assert(manager.hasPerspective("p1_cam"));
    assert(manager.hasPerspective("p2_cam"));

    // Verify distinct identifiers and shared display name
    assert(manager.get(0)->getIdentifier() == "p1_cam");
    assert(manager.get(0)->getName() == "Overhead View");
    assert(manager.get(1)->getIdentifier() == "p2_cam");
    assert(manager.get(1)->getName() == "Overhead View");

    // Switching by unique identifier targets the specific perspective
    manager.switchTo("p2_cam");
    assert(manager.currentIndex() == 1);
    assert(manager.current()->getType() == PersonPerspective::PerspectiveType::Isometric);

    manager.switchTo("p1_cam");
    assert(manager.currentIndex() == 0);
    assert(manager.current()->getType() == PersonPerspective::PerspectiveType::TopDown);

    // Duplicate identifier should still be rejected
    manager.addPerspective(std::make_unique<PersonPerspective>("p1_cam", "Overhead View", PersonPerspective::PerspectiveType::FreeCamera));
    assert(manager.count() == 2);
}

} // namespace

int main() {
    test_initialization();
    test_add_perspective();
    test_switch_perspective();
    test_remove_perspective();
    test_state_management();
    test_clear();
    test_distinct_perspectives_same_spelling();

    std::puts("perspective_manager_test: ALL OK");
    return 0;
}
