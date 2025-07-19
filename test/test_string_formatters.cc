#include <doctest/doctest.h>
#include <failsafe/detail/string_utils.hh>
#include <limits>

using namespace failsafe::detail;

TEST_SUITE("string formatters") {
    TEST_CASE("uppercase formatter") {
        SUBCASE("basic string") {
            CHECK(build_message(uppercase("hello world")) == "HELLO WORLD");
            CHECK(build_message(uppercase("Test123")) == "TEST123");
        }
        
        SUBCASE("with numbers") {
            CHECK(build_message(uppercase("value:"), 42) == "VALUE: 42");
        }
        
        SUBCASE("bool values") {
            CHECK(build_message(uppercase(true)) == "TRUE");
            CHECK(build_message(uppercase(false)) == "FALSE");
        }
        
        SUBCASE("mixed content") {
            CHECK(build_message("Error:", uppercase("failed"), "at line", 42) == 
                  "Error: FAILED at line 42");
        }
    }
    
    TEST_CASE("lowercase formatter") {
        SUBCASE("basic string") {
            CHECK(build_message(lowercase("HELLO WORLD")) == "hello world");
            CHECK(build_message(lowercase("TeSt123")) == "test123");
        }
        
        SUBCASE("with filesystem path") {
            std::filesystem::path path("/HOME/USER/FILE.TXT");
            CHECK(build_message(lowercase(path)) == "/home/user/file.txt");
        }
    }
    
    TEST_CASE("hex formatter") {
        SUBCASE("basic values") {
            CHECK(build_message(hex(255)) == "0xff");
            CHECK(build_message(hex(0)) == "0");
            CHECK(build_message(hex(0xDEADBEEFU)) == "0xdeadbeef");
        }
        
        SUBCASE("with width") {
            CHECK(build_message(hex(15, 4)) == "0x000f");
            CHECK(build_message(hex(255, 2)) == "0xff");
            CHECK(build_message(hex(1, 8)) == "0x00000001");
        }
        
        SUBCASE("uppercase") {
            CHECK(build_message(hex(255, 0, true, true)) == "0xFF");
            CHECK(build_message(hex(0xABCDEFU, 0, true, true)) == "0xABCDEF");
        }
        
        SUBCASE("without base") {
            CHECK(build_message(hex(255, 0, false)) == "ff");
            CHECK(build_message(hex(15, 4, false)) == "000f");
        }
        
        SUBCASE("negative values") {
            CHECK(build_message(hex(-1, 8)) == "0xffffffff");
            CHECK(build_message(hex(int8_t(-1), 2)) == "0xff");
        }
        
        SUBCASE("pointers") {
            int value = 42;
            int* ptr = &value;
            auto result = build_message(hex(ptr));
            CHECK(result.substr(0, 2) == "0x");
            CHECK(result.length() > 2);
            
            int* null_ptr = nullptr;
            CHECK(build_message(hex(null_ptr)) == "nullptr");
        }
    }
    
    TEST_CASE("octal formatter") {
        SUBCASE("basic values") {
            CHECK(build_message(oct(8)) == "010");
            CHECK(build_message(oct(0)) == "0");
            CHECK(build_message(oct(511)) == "0777");
        }
        
        SUBCASE("file permissions") {
            CHECK(build_message(oct(0755)) == "0755");
            CHECK(build_message(oct(0644)) == "0644");
        }
        
        SUBCASE("without base") {
            CHECK(build_message(oct(8, 0, false)) == "10");
            CHECK(build_message(oct(64, 0, false)) == "100");
        }
        
        SUBCASE("with width") {
            CHECK(build_message(oct(8, 4)) == "0010");
            CHECK(build_message(oct(1, 3)) == "001");
        }
    }
    
    TEST_CASE("binary formatter") {
        SUBCASE("basic values") {
            CHECK(build_message(bin(5)) == "0b101");
            CHECK(build_message(bin(0)) == "0b0");
            CHECK(build_message(bin(255)) == "0b11111111");
        }
        
        SUBCASE("with width") {
            CHECK(build_message(bin(5, 8)) == "0b00000101");
            CHECK(build_message(bin(15, 4)) == "0b1111");
            CHECK(build_message(bin(1, 16)) == "0b0000000000000001");
        }
        
        SUBCASE("without base") {
            CHECK(build_message(bin(5, 0, false)) == "101");
            CHECK(build_message(bin(255, 8, false)) == "11111111");
        }
        
        SUBCASE("with grouping") {
            CHECK(build_message(bin(0b10101111, 0, true, 4)) == "0b1010 1111");
            CHECK(build_message(bin(0b11111111, 0, true, 4)) == "0b1111 1111");
            CHECK(build_message(bin(0b101, 8, true, 4)) == "0b0000 0101");
        }
        
        SUBCASE("different group sizes") {
            CHECK(build_message(bin(0b11111111, 0, true, 8)) == "0b11111111");
            CHECK(build_message(bin(0b11111111, 0, true, 2)) == "0b11 11 11 11");
            CHECK(build_message(bin(0b11111111, 0, true, 3)) == "0b111 111 11");
        }
    }
    
    TEST_CASE("combining formatters") {
        SUBCASE("hex with case") {
            // Note: uppercase on hex will uppercase the whole output including 'x'
            CHECK(build_message(hex(255, 0, true, true)) == "0xFF");
            CHECK(build_message(hex(255, 0, true, false)) == "0xff");
        }
        
        SUBCASE("complex message") {
            int error_code = 0x1234;
            uint8_t flags = 0b10101010;
            
            auto msg = build_message(
                "Error", hex(error_code, 4, true, true), 
                "with flags:", bin(flags, 0, true, 4),
                "octal:", oct(0755)
            );
            
            CHECK(msg == "Error 0x1234 with flags: 0b1010 1010 octal: 0755");
        }
    }
    
    TEST_CASE("edge cases") {
        SUBCASE("zero values") {
            CHECK(build_message(hex(0)) == "0");
            CHECK(build_message(oct(0)) == "0");
            CHECK(build_message(bin(0)) == "0b0");
            CHECK(build_message(hex(0, 4)) == "0x0000");
        }
        
        SUBCASE("max values") {
            CHECK(build_message(hex(std::numeric_limits<uint8_t>::max())) == "0xff");
            CHECK(build_message(hex(std::numeric_limits<uint16_t>::max())) == "0xffff");
        }
        
        SUBCASE("empty strings") {
            CHECK(build_message(uppercase("")) == "");
            CHECK(build_message(lowercase("")) == "");
        }
    }
}