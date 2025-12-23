//
// Unit tests for the failsafe logger
//

// Set minimum log level to TRACE for testing
#define LOGGER_MIN_LEVEL 0

#include <doctest/doctest.h>
#include <failsafe/logger.hh>
#include <failsafe/detail/string_utils.hh>
#include <sstream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <mutex>

using namespace failsafe;
using namespace failsafe::detail;
using namespace std::chrono_literals;

// Custom test backend that captures log messages
class TestBackend {
    public:
        struct LogEntry {
            int level;
            std::string category;
            std::string message;
            const char* file;
            int line;
            std::thread::id thread_id;
        };

        void operator()(int level, const char* category, const char* file, int line,
                        const std::string& message) {
            std::lock_guard <std::mutex> lock(mutex_);
            entries_.push_back({level, std::string(category), message, file, line, std::this_thread::get_id()});
        }

        const std::vector <LogEntry>& entries() const { return entries_; }

        void clear() {
            std::lock_guard <std::mutex> lock(mutex_);
            entries_.clear();
        }

        size_t count() const {
            std::lock_guard <std::mutex> lock(mutex_);
            return entries_.size();
        }

        bool has_entry_with_message(const std::string& msg) const {
            std::lock_guard <std::mutex> lock(mutex_);
            return std::any_of(entries_.begin(), entries_.end(),
                               [&msg](const LogEntry& e) { return e.message == msg; });
        }

        bool has_entry_with_level(int level) const {
            std::lock_guard <std::mutex> lock(mutex_);
            return std::any_of(entries_.begin(), entries_.end(),
                               [level](const LogEntry& e) { return e.level == level; });
        }

    private:
        mutable std::mutex mutex_;
        std::vector <LogEntry> entries_;
};

// Test fixture to manage logger state

class LoggerTestFixture {
    public:
        LoggerTestFixture() {
            // Save original backend and level
            original_level_ = failsafe::logger::get_config().min_level.load();
            original_enabled_ = failsafe::logger::get_config().enabled.load();

            // Set up test backend
            test_backend_ = std::make_shared <TestBackend>();
            failsafe::logger::set_backend([this](int level, const char* category, const char* file,
                                                 int line, const std::string& message) {
                (*test_backend_)(level, category, file, line, message);
            });
            failsafe::logger::set_enabled(true);
            failsafe::logger::set_min_level(LOGGER_LEVEL_TRACE);
        }

        ~LoggerTestFixture() {
            // Restore original state
            failsafe::logger::reset_backend();
            failsafe::logger::set_min_level(original_level_);
            failsafe::logger::set_enabled(original_enabled_);
        }

        TestBackend& backend() { return *test_backend_; }

    private:
        std::shared_ptr <TestBackend> test_backend_;
        int original_level_;
        bool original_enabled_;
};

TEST_SUITE("Logger") {
    TEST_CASE("Basic log level macros") {
        LoggerTestFixture fixture;
        auto& backend = fixture.backend();

        SUBCASE("LOG_TRACE") {
            LOG_TRACE("Test trace message");
            CHECK(backend.count() == 1);
            CHECK(backend.entries()[0].level == LOGGER_LEVEL_TRACE);
            CHECK(backend.entries()[0].message == "Test trace message");
            CHECK(backend.entries()[0].category == "Application");
        }

        SUBCASE("LOG_DEBUG") {
            LOG_DEBUG("Test debug message");
            CHECK(backend.count() == 1);
            CHECK(backend.entries()[0].level == LOGGER_LEVEL_DEBUG);
            CHECK(backend.entries()[0].message == "Test debug message");
        }

        SUBCASE("LOG_INFO") {
            LOG_INFO("Test info message");
            CHECK(backend.count() == 1);
            CHECK(backend.entries()[0].level == LOGGER_LEVEL_INFO);
            CHECK(backend.entries()[0].message == "Test info message");
        }

        SUBCASE("LOG_WARN") {
            LOG_WARN("Test warning message");
            CHECK(backend.count() == 1);
            CHECK(backend.entries()[0].level == LOGGER_LEVEL_WARN);
            CHECK(backend.entries()[0].message == "Test warning message");
        }

        SUBCASE("LOG_ERROR") {
            LOG_ERROR("Test error message");
            CHECK(backend.count() == 1);
            CHECK(backend.entries()[0].level == LOGGER_LEVEL_ERROR);
            CHECK(backend.entries()[0].message == "Test error message");
        }

        SUBCASE("LOG_FATAL") {
            LOG_FATAL("Test fatal message");
            CHECK(backend.count() == 1);
            CHECK(backend.entries()[0].level == LOGGER_LEVEL_FATAL);
            CHECK(backend.entries()[0].message == "Test fatal message");
        }
    }

    TEST_CASE("Variadic message building") {
        LoggerTestFixture fixture;
        auto& backend = fixture.backend();

        SUBCASE("Multiple arguments") {
            LOG_INFO("Value: ", 42, ", Name: ", "test", ", Flag: ", true);
            CHECK(backend.count() == 1);
            CHECK(backend.entries()[0].message == "Value:  42 , Name:  test , Flag:  true");
        }

        SUBCASE("Formatting with hex/oct/bin") {
            LOG_INFO("Hex: ", hex(255), ", Oct: ", oct(64), ", Bin: ", bin(15));
            CHECK(backend.count() == 1);
            CHECK(backend.entries()[0].message == "Hex:  0xff , Oct:  0100 , Bin:  0b1111");
        }

        SUBCASE("Container formatting") {
            std::vector <int> vec{1, 2, 3};
            LOG_INFO("Vector: ", container(vec));
            CHECK(backend.count() == 1);
            CHECK(backend.entries()[0].message == "Vector:  [1, 2, 3]");
        }

        SUBCASE("Nullptr handling") {
            int* ptr = nullptr;
            LOG_INFO("Pointer: ", ptr);
            CHECK(backend.count() == 1);
            CHECK(backend.entries()[0].message == "Pointer:  nullptr");
        }
    }

    TEST_CASE("Runtime logging and level filtering") {
        LoggerTestFixture fixture;
        auto& backend = fixture.backend();

        SUBCASE("LOG_RUNTIME with different levels") {
            LOG_RUNTIME(LOGGER_LEVEL_TRACE, "Trace via runtime");
            LOG_RUNTIME(LOGGER_LEVEL_DEBUG, "Debug via runtime");
            LOG_RUNTIME(LOGGER_LEVEL_INFO, "Info via runtime");
            LOG_RUNTIME(LOGGER_LEVEL_WARN, "Warn via runtime");
            LOG_RUNTIME(LOGGER_LEVEL_ERROR, "Error via runtime");
            LOG_RUNTIME(LOGGER_LEVEL_FATAL, "Fatal via runtime");

            CHECK(backend.count() == 6);
            CHECK(backend.has_entry_with_message("Trace via runtime"));
            CHECK(backend.has_entry_with_message("Fatal via runtime"));
        }

        SUBCASE("Level filtering") {
            logger::set_min_level(LOGGER_LEVEL_WARN);

            LOG_TRACE("Should not appear");
            LOG_DEBUG("Should not appear");
            LOG_INFO("Should not appear");
            LOG_WARN("Should appear");
            LOG_ERROR("Should appear");

            CHECK(backend.count() == 2);
            CHECK(!backend.has_entry_with_message("Should not appear"));
            CHECK(backend.has_entry_with_message("Should appear"));
        }

        SUBCASE("Runtime level checking") {
            logger::set_min_level(LOGGER_LEVEL_INFO);

            CHECK(!logger::is_level_enabled(LOGGER_LEVEL_TRACE));
            CHECK(!logger::is_level_enabled(LOGGER_LEVEL_DEBUG));
            CHECK(logger::is_level_enabled(LOGGER_LEVEL_INFO));
            CHECK(logger::is_level_enabled(LOGGER_LEVEL_WARN));
            CHECK(logger::is_level_enabled(LOGGER_LEVEL_ERROR));
            CHECK(logger::is_level_enabled(LOGGER_LEVEL_FATAL));
        }

        SUBCASE("Disable/enable logging") {
            logger::set_enabled(false);
            LOG_INFO("Should not appear when disabled");
            CHECK(backend.count() == 0);

            logger::set_enabled(true);
            LOG_INFO("Should appear when enabled");
            CHECK(backend.count() == 1);
        }
    }

    TEST_CASE("Category-based logging") {
        LoggerTestFixture fixture;
        auto& backend = fixture.backend();

        SUBCASE("Basic category logging") {
            LOG_CAT_INFO("network", "Network message");
            LOG_CAT_ERROR("database", "Database error");

            CHECK(backend.count() == 2);
            CHECK(backend.entries()[0].category == "network");
            CHECK(backend.entries()[0].message == "Network message");
            CHECK(backend.entries()[1].category == "database");
            CHECK(backend.entries()[1].message == "Database error");
        }

        SUBCASE("All levels with categories") {
            LOG_CAT_TRACE("cat1", "Trace msg");
            LOG_CAT_DEBUG("cat2", "Debug msg");
            LOG_CAT_INFO("cat3", "Info msg");
            LOG_CAT_WARN("cat4", "Warn msg");
            LOG_CAT_ERROR("cat5", "Error msg");
            LOG_CAT_FATAL("cat6", "Fatal msg");

            CHECK(backend.count() == 6);
            for (size_t i = 0; i < 6; ++i) {
                CHECK(backend.entries()[i].category == "cat" + std::to_string(i + 1));
            }
        }

        SUBCASE("Runtime category logging") {
            LOG_CAT_RUNTIME(LOGGER_LEVEL_INFO, "runtime_cat", "Runtime category message");
            CHECK(backend.count() == 1);
            CHECK(backend.entries()[0].category == "runtime_cat");
            CHECK(backend.entries()[0].level == LOGGER_LEVEL_INFO);
        }
    }

    TEST_CASE("Conditional logging") {
        LoggerTestFixture fixture;
        auto& backend = fixture.backend();

        SUBCASE("LOG_IF with true condition") {
            bool condition = true;
            LOG_IF(condition, LOGGER_LEVEL_INFO, "Conditional message - true");
            CHECK(backend.count() == 1);
            CHECK(backend.entries()[0].message == "Conditional message - true");
        }

        SUBCASE("LOG_IF with false condition") {
            bool condition = false;
            LOG_IF(condition, LOGGER_LEVEL_INFO, "Should not appear");
            CHECK(backend.count() == 0);
        }

        SUBCASE("LOG_CAT_IF") {
            LOG_CAT_IF(2 > 1, LOGGER_LEVEL_WARN, "math", "2 is greater than 1");
            LOG_CAT_IF(1 > 2, LOGGER_LEVEL_WARN, "math", "Should not appear");

            CHECK(backend.count() == 1);
            CHECK(backend.entries()[0].category == "math");
            CHECK(backend.entries()[0].message == "2 is greater than 1");
        }

        SUBCASE("Complex condition expressions") {
            int x = 5, y = 10;
            LOG_IF(x < y && y > 0, LOGGER_LEVEL_DEBUG, "Complex condition: x=", x, ", y=", y);
            CHECK(backend.count() == 1);
            CHECK(backend.entries()[0].message == "Complex condition: x= 5 , y= 10");
        }
    }

    TEST_CASE("Backend switching") {
        LoggerTestFixture fixture;
        auto& original_backend = fixture.backend();

        SUBCASE("Switch to new backend") {
            LOG_INFO("Message to original backend");
            CHECK(original_backend.count() == 1);

            auto new_backend = std::make_shared <TestBackend>();
            logger::set_backend([new_backend](int level, const char* category, const char* file,
                                              int line, const std::string& message) {
                (*new_backend)(level, category, file, line, message);
            });

            LOG_INFO("Message to new backend");
            CHECK(original_backend.count() == 1); // Still 1
            CHECK(new_backend->count() == 1);
            CHECK(new_backend->entries()[0].message == "Message to new backend");
        }

        SUBCASE("Reset to default backend") {
            LOG_INFO("Before reset");
            CHECK(original_backend.count() == 1);

            logger::reset_backend();
            LOG_INFO("After reset"); // Goes to default cerr backend

            CHECK(original_backend.count() == 1); // No new messages
        }

        SUBCASE("Null backend handling") {
            logger::set_backend(nullptr);
            LOG_INFO("Message with null backend"); // Should not crash
            // Can't verify output as it goes to default backend
        }
    }

    TEST_CASE("File and line information") {
        LoggerTestFixture fixture;
        auto& backend = fixture.backend();

        int line = __LINE__ + 1;
        LOG_INFO("Test message");

        CHECK(backend.count() == 1);
        CHECK(backend.entries()[0].file != nullptr);
        CHECK(std::string(backend.entries()[0].file).find("test_logger.cc") != std::string::npos);
        CHECK(backend.entries()[0].line == line);
    }

    TEST_CASE("Thread safety") {
        LoggerTestFixture fixture;
        auto& backend = fixture.backend();

        constexpr int num_threads = 4;
        constexpr int messages_per_thread = 100;
        std::vector <std::thread> threads;
        std::atomic <int> start_flag{0};

        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([t, &start_flag]() {
                // Wait for all threads to be ready
                start_flag.fetch_add(1);
                while (start_flag.load() < num_threads) {
                    std::this_thread::yield();
                }

                // Log messages
                for (int i = 0; i < messages_per_thread; ++i) {
                    LOG_INFO("Thread ", t, " message ", i);
                }
            });
        }

        // Join all threads
        for (auto& thread : threads) {
            thread.join();
        }

        // Verify all messages were logged
        CHECK(backend.count() == num_threads * messages_per_thread);

        // Verify each thread logged the correct number of messages
        std::map <std::thread::id, int> thread_counts;
        for (const auto& entry : backend.entries()) {
            thread_counts[entry.thread_id]++;
        }

        CHECK(thread_counts.size() == num_threads);
        for (const auto& [tid, count] : thread_counts) {
            CHECK(count == messages_per_thread);
        }
    }

    TEST_CASE("Special types logging") {
        LoggerTestFixture fixture;
        auto& backend = fixture.backend();

        SUBCASE("Chrono durations") {
            auto duration = 42ms;
            LOG_INFO("Duration: ", duration);
            CHECK(backend.count() == 1);
            CHECK(backend.entries()[0].message == "Duration:  42ms");
        }

        SUBCASE("std::optional") {
            std::optional <int> opt_with_value = 42;
            std::optional <int> opt_empty;

            LOG_INFO("Optional with value: ", opt_with_value);
            LOG_INFO("Empty optional: ", opt_empty);

            CHECK(backend.count() == 2);
            CHECK(backend.entries()[0].message == "Optional with value:  42");
            CHECK(backend.entries()[1].message == "Empty optional:  nullopt");
        }

        SUBCASE("Filesystem paths") {
            std::filesystem::path path("/tmp/test.txt");
            LOG_INFO("Path: ", path);
            CHECK(backend.count() == 1);
            CHECK(backend.entries()[0].message == "Path:  /tmp/test.txt");
        }
    }

    TEST_CASE("Error conditions") {
        LoggerTestFixture fixture;
        auto& backend = fixture.backend();

        SUBCASE("Empty messages") {
            LOG_INFO("");
            CHECK(backend.count() == 1);
            CHECK(backend.entries()[0].message.empty());
        }

        SUBCASE("Very long messages") {
            std::string long_str(1000, 'x');
            LOG_INFO("Long: ", long_str);
            CHECK(backend.count() == 1);
            CHECK(backend.entries()[0].message == "Long:  " + long_str);
        }
    }

    TEST_CASE("Default cerr backend") {
        // Save original state
        auto original_level = logger::get_config().min_level.load();
        auto original_enabled = logger::get_config().enabled.load();
        
        // Test that default backend works
        logger::reset_backend();
        logger::set_enabled(true);
        logger::set_min_level(LOGGER_LEVEL_TRACE);

        // Capture cerr output
        std::stringstream captured;
        std::streambuf* old_cerr = std::cerr.rdbuf(captured.rdbuf());

        LOG_ERROR("Test error to cerr");

        // Restore cerr
        std::cerr.rdbuf(old_cerr);

        std::string output = captured.str();
        CHECK(!output.empty());
        CHECK(output.find("Test error to cerr") != std::string::npos);
        
        // Restore original state
        logger::set_min_level(original_level);
        logger::set_enabled(original_enabled);
    }
}
