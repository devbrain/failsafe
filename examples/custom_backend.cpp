/**
 * @file custom_backend.cpp
 * @brief Implementing a custom logger backend
 * 
 * This example shows how to create custom logger backends for different
 * logging destinations such as files, syslog, or remote services.
 */

#include <failsafe/logger.hh>
#include <failsafe/logger/backend/cerr_backend.hh>
#include <fstream>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <iomanip>
#include <sstream>
#include <ctime>

using namespace failsafe;

namespace {
    inline std::tm* safe_localtime(const std::time_t* time, std::tm* result) {
#ifdef _WIN32
        return localtime_s(result, time) == 0 ? result : nullptr;
#else
        return localtime_r(time, result);
#endif
    }
}

// Example 1: Simple File Backend
class FileBackend {
    std::ofstream file_;
    std::mutex mutex_;
    
public:
    explicit FileBackend(const std::string& filename) 
        : file_(filename, std::ios::app) {
        if (!file_) {
            throw std::runtime_error("Failed to open log file: " + filename);
        }
    }
    
    void operator()(int level, const char* category, 
                    const char* file, int line, 
                    const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Format timestamp
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
        safe_localtime(&time_t, &tm);

        file_ << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
              << " [" << logger::internal::level_to_string(level) << "]"
              << " [" << category << "]"
              << " " << file << ":" << line
              << " - " << message << std::endl;
    }
};

// Example 2: Async Queue Backend
class AsyncQueueBackend {
    struct LogEntry {
        int level;
        std::string category;
        std::string file;
        int line;
        std::string message;
        std::chrono::system_clock::time_point timestamp;
    };
    
    std::queue<LogEntry> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> running_{true};
    std::ofstream file_;
    std::thread worker_;
    
    void worker_thread() {
        while (running_ || !queue_.empty()) {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return !queue_.empty() || !running_; });
            
            while (!queue_.empty()) {
                auto entry = std::move(queue_.front());
                queue_.pop();
                lock.unlock();
                
                // Write to file
                auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
                std::tm tm{};
                safe_localtime(&time_t, &tm);
                file_ << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
                      << " [" << logger::internal::level_to_string(entry.level) << "]"
                      << " [" << entry.category << "]"
                      << " " << entry.file << ":" << entry.line
                      << " - " << entry.message << std::endl;
                
                lock.lock();
            }
        }
    }
    
public:
    explicit AsyncQueueBackend(const std::string& filename) 
        : file_(filename, std::ios::app)
        , worker_(&AsyncQueueBackend::worker_thread, this) {
        if (!file_) {
            running_ = false;
            throw std::runtime_error("Failed to open log file: " + filename);
        }
    }
    
    ~AsyncQueueBackend() {
        running_ = false;
        cv_.notify_all();
        if (worker_.joinable()) {
            worker_.join();
        }
    }
    
    void operator()(int level, const char* category, 
                    const char* file, int line, 
                    const std::string& message) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push({level, category, file, line, message, 
                        std::chrono::system_clock::now()});
        }
        cv_.notify_one();
    }
};

// Example 3: Filtering Backend
class FilteringBackend {
    logger::LoggerBackend wrapped_;
    int min_level_;
    std::set<std::string> allowed_categories_;
    
public:
    FilteringBackend(logger::LoggerBackend wrapped, int min_level)
        : wrapped_(std::move(wrapped))
        , min_level_(min_level) {}
    
    void add_category(const std::string& category) {
        allowed_categories_.insert(category);
    }
    
    void operator()(int level, const char* category, 
                    const char* file, int line, 
                    const std::string& message) {
        // Filter by level
        if (level < min_level_) return;
        
        // Filter by category if categories are specified
        if (!allowed_categories_.empty() && 
            allowed_categories_.find(category) == allowed_categories_.end()) {
            return;
        }
        
        // Forward to wrapped backend
        wrapped_(level, category, file, line, message);
    }
};

// Example 4: Multi-Backend (Tee)
class MultiBackend {
    std::vector<logger::LoggerBackend> backends_;
    
public:
    void add_backend(logger::LoggerBackend backend) {
        backends_.push_back(std::move(backend));
    }
    
    void operator()(int level, const char* category, 
                    const char* file, int line, 
                    const std::string& message) {
        for (auto& backend : backends_) {
            backend(level, category, file, line, message);
        }
    }
};

// Example 5: JSON Backend
class JsonBackend {
    std::ofstream file_;
    std::mutex mutex_;
    bool first_entry_ = true;
    
public:
    explicit JsonBackend(const std::string& filename) 
        : file_(filename, std::ios::app) {
        if (!file_) {
            throw std::runtime_error("Failed to open log file: " + filename);
        }
        file_ << "[\n";
    }
    
    ~JsonBackend() {
        file_ << "\n]\n";
    }
    
    void operator()(int level, const char* category, 
                    const char* file, int line, 
                    const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!first_entry_) {
            file_ << ",\n";
        }
        first_entry_ = false;
        
        // Escape JSON strings
        auto escape_json = [](const std::string& s) {
            std::string result;
            for (char c : s) {
                switch (c) {
                    case '"': result += "\\\""; break;
                    case '\\': result += "\\\\"; break;
                    case '\n': result += "\\n"; break;
                    case '\r': result += "\\r"; break;
                    case '\t': result += "\\t"; break;
                    default: result += c;
                }
            }
            return result;
        };
        
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        
        file_ << "  {\n"
              << "    \"timestamp\": " << ms << ",\n"
              << "    \"level\": \"" << logger::internal::level_to_string(level) << "\",\n"
              << "    \"category\": \"" << escape_json(category) << "\",\n"
              << "    \"file\": \"" << escape_json(file) << "\",\n"
              << "    \"line\": " << line << ",\n"
              << "    \"message\": \"" << escape_json(message) << "\"\n"
              << "  }";
    }
};

// Demonstration
void demonstrate_backends() {
    // Example 1: File Backend
    {
        std::cout << "\n=== File Backend Example ===\n";
        FileBackend file_backend("app.log");
        logger::set_backend(std::ref(file_backend));
        
        LOG_INFO("Application started");
        LOG_ERROR("Example error message");
        
        std::cout << "Logs written to app.log\n";
    }
    
    // Example 2: Async Queue Backend
    {
        std::cout << "\n=== Async Queue Backend Example ===\n";
        AsyncQueueBackend async_backend("async.log");
        logger::set_backend(std::ref(async_backend));
        
        // Generate many log messages quickly
        for (int i = 0; i < 100; ++i) {
            LOG_DEBUG("Async message", i);
        }
        
        std::cout << "100 messages queued asynchronously to async.log\n";
    }
    
    // Example 3: Filtering Backend
    {
        std::cout << "\n=== Filtering Backend Example ===\n";
        
        // Wrap the cerr backend with filtering
        FilteringBackend filter(logger::backends::make_cerr_backend(true, false, true), 
                               LOGGER_LEVEL_WARN);
        filter.add_category("Security");
        filter.add_category("Database");
        
        logger::set_backend(std::ref(filter));
        
        LOG_CAT_DEBUG("Network", "This won't appear (wrong category)");
        LOG_CAT_INFO("Security", "This won't appear (level too low)");
        LOG_CAT_ERROR("Security", "This WILL appear");
        LOG_CAT_WARN("Database", "This WILL appear");
    }
    
    // Example 4: Multi-Backend
    {
        std::cout << "\n=== Multi-Backend Example ===\n";
        
        MultiBackend multi;
        
        // Add console output
        multi.add_backend(logger::backends::make_cerr_backend(false, false, true));
        
        // Add file output
        FileBackend file("multi.log");
        multi.add_backend(std::ref(file));
        
        logger::set_backend(std::ref(multi));
        
        LOG_INFO("This appears in both console and file");
    }
    
    // Example 5: JSON Backend
    {
        std::cout << "\n=== JSON Backend Example ===\n";
        JsonBackend json_backend("events.json");
        logger::set_backend(std::ref(json_backend));
        
        LOG_INFO("User login", "user_id:", 12345);
        LOG_WARN("High memory usage", "percent:", 85);
        LOG_ERROR("Database connection failed", "retry_count:", 3);
        
        std::cout << "JSON logs written to events.json\n";
    }
    
    // Reset to default
    logger::reset_backend();
}

int main() {
    std::cout << "=== Custom Logger Backend Examples ===\n";
    
    try {
        demonstrate_backends();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    std::cout << "\n=== All examples completed ===\n";
    return 0;
}