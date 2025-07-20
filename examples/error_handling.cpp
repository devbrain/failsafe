/**
 * @file error_handling.cpp
 * @brief Comprehensive error handling patterns
 * 
 * This example demonstrates various error handling patterns using
 * Failsafe's enforcement and exception features.
 */

#include <failsafe/failsafe.hh>
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <optional>
#include <cmath>

using namespace failsafe;

// Example 1: Resource Management with RAII and Enforcement
class FileHandler {
    FILE* file_;
    
public:
    explicit FileHandler(const std::string& filename, const char* mode) {
        file_ = ENFORCE(fopen(filename.c_str(), mode))
            ("Failed to open file:", filename, "with mode:", mode);
    }
    
    ~FileHandler() {
        if (file_) {
            fclose(file_);
        }
    }
    
    void write(const std::string& data) {
        auto written = fwrite(data.c_str(), 1, data.size(), file_);
        ENFORCE_EQ(written, data.size())
            ("Failed to write all data. Written:", written, "Expected:", data.size());
    }
    
    std::string read(size_t size) {
        std::string buffer(size, '\0');
        auto read_bytes = fread(buffer.data(), 1, size, file_);
        ENFORCE_GT(read_bytes, 0u)("Failed to read from file");
        buffer.resize(read_bytes);
        return buffer;
    }
};

// Example 2: Network Operations with Detailed Error Context
class NetworkClient {
    int socket_fd_ = -1;
    
public:
    void connect(const std::string& host, int port) {
        try {
            // Validate inputs
            ENFORCE(!host.empty())("Host cannot be empty");
            ENFORCE_IN_RANGE(port, 1, 65535)("Invalid port number");
            
            // Simulate connection (would be real socket code)
            socket_fd_ = -1; // Simulate failure
            ENFORCE_NE(socket_fd_, -1)("Failed to create socket");
            
        } catch (...) {
            // Add context about what we were trying to do
            THROW(std::runtime_error, 
                  "Failed to connect to", host, ":", port);
        }
    }
    
    void send_request(const std::string& endpoint, const std::string& data) {
        try {
            ENFORCE_GE(socket_fd_, 0)("Not connected");
            ENFORCE(!endpoint.empty())("Endpoint cannot be empty");
            ENFORCE_LE(data.size(), 1024 * 1024)("Request too large");
            
            // Simulate sending (would be real socket code)
            
        } catch (...) {
            THROW(std::runtime_error,
                  "Failed to send request to", endpoint,
                  "with", data.size(), "bytes of data");
        }
    }
};

// Example 3: Configuration Validation
struct AppConfig {
    std::string db_host;
    int db_port;
    std::string api_key;
    int max_connections;
    double timeout_seconds;
    
    void validate() {
        try {
            // Database settings
            ENFORCE(!db_host.empty())("Database host is required");
            ENFORCE_IN_RANGE(db_port, 1, 65535)("Invalid database port");
            
            // API settings
            ENFORCE(!api_key.empty())("API key is required");
            ENFORCE_GE(api_key.length(), 32)("API key too short");
            
            // Connection settings
            ENFORCE_IN_RANGE(max_connections, 1, 1000)
                ("Max connections must be between 1 and 1000");
            
            // Timeout settings
            ENFORCE_GT(timeout_seconds, 0.0)("Timeout must be positive");
            ENFORCE_LE(timeout_seconds, 300.0)("Timeout too large");
            
        } catch (...) {
            THROW(std::invalid_argument, "Invalid configuration");
        }
    }
};

// Example 4: Mathematical Operations with Domain Validation
class Calculator {
public:
    double safe_divide(double a, double b) {
        ENFORCE_NE(b, 0.0)("Division by zero attempted:", a, "/", b);
        return a / b;
    }
    
    double safe_sqrt(double x) {
        ENFORCE_GE(x, 0.0)("Cannot take square root of negative number:", x);
        return std::sqrt(x);
    }
    
    double safe_log(double x) {
        ENFORCE_GT(x, 0.0)("Logarithm requires positive argument:", x);
        return std::log(x);
    }
};

// Example 5: State Machine with Enforcement
class OrderStateMachine {
public:
    enum State {
        CREATED,
        CONFIRMED,
        SHIPPED,
        DELIVERED,
        CANCELLED
    };
    
private:
    State state_ = CREATED;
    
    void enforce_transition(State from, State to) {
        ENFORCE_EQ(state_, from)
            ("Invalid state transition. Current:", state_name(state_),
             "Expected:", state_name(from), "Target:", state_name(to));
    }
    
    static const char* state_name(State s) {
        switch (s) {
            case CREATED: return "CREATED";
            case CONFIRMED: return "CONFIRMED";
            case SHIPPED: return "SHIPPED";
            case DELIVERED: return "DELIVERED";
            case CANCELLED: return "CANCELLED";
            default: return "UNKNOWN";
        }
    }
    
public:
    void confirm() {
        try {
            enforce_transition(CREATED, CONFIRMED);
            state_ = CONFIRMED;
            LOG_INFO("Order confirmed");
        } catch (...) {
            THROW(std::logic_error, "Cannot confirm order in current state");
        }
    }
    
    void ship() {
        try {
            enforce_transition(CONFIRMED, SHIPPED);
            state_ = SHIPPED;
            LOG_INFO("Order shipped");
        } catch (...) {
            THROW(std::logic_error, "Cannot ship order in current state");
        }
    }
    
    void deliver() {
        try {
            enforce_transition(SHIPPED, DELIVERED);
            state_ = DELIVERED;
            LOG_INFO("Order delivered");
        } catch (...) {
            THROW(std::logic_error, "Cannot deliver order in current state");
        }
    }
    
    void cancel() {
        try {
            ENFORCE(state_ == CREATED || state_ == CONFIRMED)
                ("Can only cancel orders that haven't shipped");
            state_ = CANCELLED;
            LOG_INFO("Order cancelled");
        } catch (...) {
            THROW(std::logic_error, "Cannot cancel order in current state");
        }
    }
};

// Example 6: Complex Business Logic with Multiple Validations
class BankAccount {
    double balance_;
    bool frozen_ = false;
    
public:
    explicit BankAccount(double initial_balance) {
        ENFORCE_GE(initial_balance, 0.0)("Initial balance cannot be negative");
        balance_ = initial_balance;
    }
    
    void deposit(double amount) {
        try {
            ENFORCE(!frozen_)("Account is frozen");
            ENFORCE_GT(amount, 0.0)("Deposit amount must be positive");
            ENFORCE_LE(amount, 1000000.0)("Deposit amount too large");
            
            balance_ += amount;
            LOG_INFO("Deposited:", amount, "New balance:", balance_);
            
        } catch (...) {
            THROW(std::runtime_error, "Deposit failed for amount:", amount);
        }
    }
    
    void withdraw(double amount) {
        try {
            ENFORCE(!frozen_)("Account is frozen");
            ENFORCE_GT(amount, 0.0)("Withdrawal amount must be positive");
            ENFORCE_LE(amount, balance_)("Insufficient funds");
            ENFORCE_LE(amount, 5000.0)("Daily withdrawal limit exceeded");
            
            balance_ -= amount;
            LOG_INFO("Withdrew:", amount, "New balance:", balance_);
            
        } catch (...) {
            THROW(std::runtime_error, 
                  "Withdrawal failed. Amount:", amount, 
                  "Available:", balance_);
        }
    }
    
    void freeze() {
        frozen_ = true;
        LOG_WARN("Account frozen");
    }
};

// Demonstration
void demonstrate_error_handling() {
    // Example 1: File Operations
    std::cout << "\n=== File Operations Example ===\n";
    try {
        FileHandler file("test.txt", "w");
        file.write("Hello, Failsafe!");
        std::cout << "File written successfully\n";
    } catch (const std::exception& e) {
        std::cout << "File error: " << e.what() << "\n";
    }
    
    // Example 2: Network Operations
    std::cout << "\n=== Network Operations Example ===\n";
    try {
        NetworkClient client;
        client.connect("api.example.com", 443);
    } catch (const std::exception& e) {
        std::cout << "Network error trace:\n";
        std::cout << exception::get_nested_trace(e);
    }
    
    // Example 3: Configuration Validation
    std::cout << "\n=== Configuration Validation Example ===\n";
    try {
        AppConfig config{
            "",              // db_host - Invalid: empty
            5432,            // db_port
            "short_key",     // api_key - Invalid: too short
            100,             // max_connections
            30.0             // timeout_seconds
        };
        config.validate();
    } catch (const std::exception& e) {
        std::cout << "Config validation error:\n";
        std::cout << exception::get_nested_trace(e);
    }
    
    // Example 4: Mathematical Operations
    std::cout << "\n=== Mathematical Operations Example ===\n";
    Calculator calc;
    try {
        auto result = calc.safe_divide(10, 0);
    } catch (const std::exception& e) {
        std::cout << "Math error: " << e.what() << "\n";
    }
    
    // Example 5: State Machine
    std::cout << "\n=== State Machine Example ===\n";
    try {
        OrderStateMachine order;
        order.confirm();
        order.deliver();  // Error - should ship first
    } catch (const std::exception& e) {
        std::cout << "State error:\n";
        std::cout << exception::get_nested_trace(e);
    }
    
    // Example 6: Banking Operations
    std::cout << "\n=== Banking Operations Example ===\n";
    try {
        BankAccount account(1000.0);
        account.deposit(500.0);
        account.withdraw(2000.0);  // Error - insufficient funds
    } catch (const std::exception& e) {
        std::cout << "Banking error:\n";
        std::cout << exception::get_nested_trace(e);
    }
}

int main() {
    std::cout << "=== Comprehensive Error Handling Examples ===\n";
    
    try {
        demonstrate_error_handling();
    } catch (const std::exception& e) {
        LOG_FATAL("Unexpected error:", e.what());
        return 1;
    }
    
    std::cout << "\n=== Examples completed ===\n";
    return 0;
}