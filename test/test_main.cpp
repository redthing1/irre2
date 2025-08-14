// Test main entry point for IRRE tests
// This provides the main() function for our tests

#include <catch2/catch_session.hpp>

int main(int argc, char* argv[]) { return Catch::Session().run(argc, argv); }