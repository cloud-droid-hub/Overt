// string.cpp
#include "string.h"
#include <cstring>  // For custom_strlen, custom_strcpy, and custom_strcat implementations
#include <stdexcept>  // For std::out_of_range
#include <android/log.h>
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "lxz", __VA_ARGS__)

namespace nonstd {

// Custom implementation of strlen (get the length of the string)
    size_t string::custom_strlen(const char* str) const {
        size_t length = 0;
        while (str[length] != '\0') {
            ++length;
        }
        return length;
    }

// Custom implementation of strcpy (copy a C-string)
    char* string::custom_strcpy(char* dest, const char* src) const {
        char* d = dest;
        while ((*d++ = *src++) != '\0') {}  // Copy characters
        return dest;
    }

// Custom implementation of strcat (concatenate two C-strings)
    char* string::custom_strcat(char* dest, const char* src) const {
        char* d = dest;
        while (*d != '\0') {  // Find the end of the first string
            ++d;
        }
        while ((*d++ = *src++) != '\0') {}  // Append the second string
        return dest;
    }

// Default constructor
    string::string() : data(new char[1]{'\0'}), len(0) {}

// Constructor from C-string
    string::string(const char* str) {
        len = custom_strlen(str);  // Use custom strlen
        data = new char[len + 1];  // +1 for null terminator
        custom_strcpy(data, str);  // Use custom strcpy
    }
    
    // Constructor from C-string with length
    string::string(const char* str, size_t len) {
        LOGE("nonstd::string(const char* str, size_t len) is called with len=%zu", len);
        
        if (!str) {
            LOGE("nonstd::string: str is NULL, creating empty string");
            data = new char[1]{'\0'};
            this->len = 0;
            return;
        }
        
        this->len = len;
        data = new char[len + 1];
        
        // Copy the specified length
        for (size_t i = 0; i < len; ++i) {
            data[i] = str[i];
        }
        data[len] = '\0';
        
        LOGE("nonstd::string(const char*, size_t) completed, length=%zu", this->len);
    }

// Copy constructor
    string::string(const string& other) {
        len = other.len;
        data = new char[len + 1];
        custom_strcpy(data, other.data);  // Use custom strcpy
    }

// Move constructor
    string::string(string&& other) noexcept : data(other.data), len(other.len) {
        other.data = nullptr;  // Nullify the other object's data
        other.len = 0;
    }

// Destructor
    string::~string() {
        delete[] data;
    }

// Copy assignment operator
    string& string::operator=(const string& other) {
        if (this == &other) return *this;  // Self-assignment check

        delete[] data;  // Clean up existing data

        len = other.len;
        data = new char[len + 1];
        custom_strcpy(data, other.data);  // Use custom strcpy

        return *this;
    }

// Move assignment operator
    string& string::operator=(string&& other) noexcept {
        if (this != &other) {
            delete[] data;  // Clean up existing data

            data = other.data;
            len = other.len;

            other.data = nullptr;  // Nullify the other object's data
            other.len = 0;
        }
        return *this;
    }

// Get the length of the string
    size_t string::length() const {
        return len;
    }

// Get the size of the string (same as length)
    size_t string::size() const {
        LOGE("nonstd::string.size() is called, returning %zu", len);
        return len;
    }

// Check if the string is empty
    bool string::empty() const {
        LOGE("nonstd::string.empty() is called, returning %s", (len == 0) ? "true" : "false");
        return len == 0;
    }

    // Clear the string
    void string::clear() {
        LOGE("nonstd::string.clear() is called, old length=%zu", len);
        delete[] data;
        data = new char[1]{'\0'};
        len = 0;
        LOGE("nonstd::string.clear() completed, new length=%zu", len);
    }

// Get substring starting at pos with length len
    string string::substr(size_t pos, size_t len) const {
        LOGE("nonstd::string.substr(pos=%zu, len=%zu) is called", pos, len);
        
        // Check bounds
        if (pos >= this->len) {
            LOGE("nonstd::string.substr: pos %zu >= length %zu, throwing out_of_range", pos, this->len);
            throw std::out_of_range("string::substr");
        }
        
        // Calculate actual length
        size_t actual_len = len;
        if (len == npos || pos + len > this->len) {
            actual_len = this->len - pos;
        }
        
        LOGE("nonstd::string.substr: creating substring of length %zu", actual_len);
        
        // Create new string with substring
        string result;
        if (actual_len > 0) {
            result.data = new char[actual_len + 1];
            result.capacity = actual_len + 1;
            
            // Copy substring
            for (size_t i = 0; i < actual_len; ++i) {
                result.data[i] = data[pos + i];
            }
            result.data[actual_len] = '\0';
            result.len = actual_len;
        }
        
        LOGE("nonstd::string.substr: returning substring '%s'", result.c_str());
        return result;
    }

// Access the underlying C-string
    const char* string::c_str() const {
        LOGE("nonstd::string.c_str() is called");
        return data;
    }

// Operator[] for character access
    char& string::operator[](size_t index) {
        if (index >= len) {
            throw std::out_of_range("Index out of range");
        }
        return data[index];
    }

// Const version of operator[] for read-only access
    const char& string::operator[](size_t index) const {
        if (index >= len) {
            throw std::out_of_range("Index out of range");
        }
        return data[index];
    }

// Concatenation operator (A + B)
    string string::operator+(const string& other) const {
        string result;
        result.len = len + other.len;
        result.data = new char[result.len + 1];  // +1 for null terminator

        custom_strcpy(result.data, data);  // Copy this string
        custom_strcat(result.data, other.data);  // Concatenate the other string

        return result;
    }

// Friend function for const char* + string
    string operator+(const char* str, const string& other) {
        LOGE("nonstd::operator+(const char*, const string&) called, str='%s', other='%s'", str, other.c_str());
        
        size_t str_len = 0;
        while (str[str_len] != '\0') {
            ++str_len;
        }
        
        string result;
        result.len = str_len + other.len;
        result.data = new char[result.len + 1];  // +1 for null terminator
        
        // Copy the C-string first
        char* dest = result.data;
        const char* src = str;
        while ((*dest++ = *src++) != '\0') {}
        --dest; // Move back one position (over the null terminator)
        
        // Then copy the string
        src = other.data;
        while ((*dest++ = *src++) != '\0') {}
        
        LOGE("nonstd::operator+(const char*, const string&) done, result='%s'", result.c_str());
        return result;
    }

// Compound assignment operator (+=) for string
    string& string::operator+=(const string& other) {
        LOGE("nonstd::string.operator+=(const string&) called, this len=%zu, other len=%zu", len, other.len);
        
        size_t new_len = len + other.len;
        char* new_data = new char[new_len + 1];  // +1 for null terminator
        
        custom_strcpy(new_data, data);  // Copy this string
        custom_strcat(new_data, other.data);  // Concatenate the other string
        
        delete[] data;  // Clean up old data
        data = new_data;
        len = new_len;
        
        LOGE("nonstd::string.operator+=(const string&) done, new len=%zu", len);
        return *this;
    }

// Compound assignment operator (+=) for C-string
    string& string::operator+=(const char* str) {
        LOGE("nonstd::string.operator+=(const char*) called, this len=%zu, str len=%zu", len, custom_strlen(str));
        
        size_t str_len = custom_strlen(str);
        size_t new_len = len + str_len;
        char* new_data = new char[new_len + 1];  // +1 for null terminator
        
        custom_strcpy(new_data, data);  // Copy this string
        custom_strcat(new_data, str);   // Concatenate the C-string
        
        delete[] data;  // Clean up old data
        data = new_data;
        len = new_len;
        
        LOGE("nonstd::string.operator+=(const char*) done, new len=%zu", len);
        return *this;
    }

// Compound assignment operator (+=) for char
    string& string::operator+=(char ch) {
        LOGE("nonstd::string.operator+=(char '%c') is called, current length=%zu", ch, len);
        
        size_t new_len = len + 1;  // Add one character
        char* new_data = new char[new_len + 1];  // +1 for null terminator
        
        custom_strcpy(new_data, data);  // Copy this string
        new_data[len] = ch;             // Append the character
        new_data[new_len] = '\0';       // Add null terminator
        
        delete[] data;  // Clean up old data
        data = new_data;
        len = new_len;
        
        LOGE("nonstd::string.operator+=(char) completed, new length=%zu", len);
        return *this;
    }

    // Equality comparison operator (==) for string
    bool string::operator==(const string& other) const {
        LOGE("nonstd::string.operator==(string) is called, comparing '%s' with '%s'", data, other.data);
        
        if (len != other.len) {
            LOGE("nonstd::string.operator==(string): lengths differ (%zu vs %zu), returning false", len, other.len);
            return false;
        }
        
        // Compare each character
        for (size_t i = 0; i < len; ++i) {
            if (data[i] != other.data[i]) {
                LOGE("nonstd::string.operator==(string): characters differ at position %zu ('%c' vs '%c'), returning false", i, data[i], other.data[i]);
                return false;
            }
        }
        
        LOGE("nonstd::string.operator==(string): strings are equal, returning true");
        return true;
    }

    // Equality comparison operator (==) for const char*
    bool string::operator==(const char* str) const {
        LOGE("nonstd::string.operator==(const char*) is called, comparing '%s' with '%s'", data, str ? str : "NULL");
        
        if (!str) {
            LOGE("nonstd::string.operator==(const char*): str is NULL, returning false");
            return false;
        }
        
        // Compare each character
        size_t i = 0;
        while (i < len && str[i] != '\0') {
            if (data[i] != str[i]) {
                LOGE("nonstd::string.operator==(const char*): characters differ at position %zu ('%c' vs '%c'), returning false", i, data[i], str[i]);
                return false;
            }
            i++;
        }
        
        // Check if both strings ended at the same time
        bool result = (i == len && str[i] == '\0');
        LOGE("nonstd::string.operator==(const char*): comparison result = %s", result ? "true" : "false");
        return result;
    }

    // Inequality comparison operator (!=) for string
    bool string::operator!=(const string& other) const {
        return !(*this == other);
    }

    // Inequality comparison operator (!=) for const char*
    bool string::operator!=(const char* str) const {
        return !(*this == str);
    }

    // Less than comparison operator (<) for string
    bool string::operator<(const string& other) const {
        LOGE("nonstd::string.operator<(string) is called, comparing '%s' with '%s'", data, other.data);
        
        size_t min_len = (len < other.len) ? len : other.len;
        
        for (size_t i = 0; i < min_len; ++i) {
            if (data[i] < other.data[i]) {
                LOGE("nonstd::string.operator<(string): '%c' < '%c' at position %zu, returning true", data[i], other.data[i], i);
                return true;
            } else if (data[i] > other.data[i]) {
                LOGE("nonstd::string.operator<(string): '%c' > '%c' at position %zu, returning false", data[i], other.data[i], i);
                return false;
            }
        }
        
        // If all characters are equal up to min_len, shorter string is less
        bool result = (len < other.len);
        LOGE("nonstd::string.operator<(string): lengths compared (%zu < %zu), returning %s", len, other.len, result ? "true" : "false");
        return result;
    }

    // Less than comparison operator (<) for const char*
    bool string::operator<(const char* str) const {
        LOGE("nonstd::string.operator<(const char*) is called, comparing '%s' with '%s'", data, str ? str : "NULL");
        
        if (!str) {
            LOGE("nonstd::string.operator<(const char*): str is NULL, returning false");
            return false;
        }
        
        size_t i = 0;
        while (i < len && str[i] != '\0') {
            if (data[i] < str[i]) {
                LOGE("nonstd::string.operator<(const char*): '%c' < '%c' at position %zu, returning true", data[i], str[i], i);
                return true;
            } else if (data[i] > str[i]) {
                LOGE("nonstd::string.operator<(const char*): '%c' > '%c' at position %zu, returning false", data[i], str[i], i);
                return false;
            }
            i++;
        }
        
        // If all characters are equal up to min_len, shorter string is less
        bool result = (i == len && str[i] != '\0');
        LOGE("nonstd::string.operator<(const char*): comparison result = %s", result ? "true" : "false");
        return result;
    }

    // Greater than comparison operator (>) for string
    bool string::operator>(const string& other) const {
        return other < *this;
    }

    // Greater than comparison operator (>) for const char*
    bool string::operator>(const char* str) const {
        if (!str) return true;
        return *this > string(str);
    }

    // Less than or equal comparison operator (<=) for string
    bool string::operator<=(const string& other) const {
        return !(other < *this);
    }

    // Less than or equal comparison operator (<=) for const char*
    bool string::operator<=(const char* str) const {
        if (!str) return false;
        return !(*this > string(str));
    }

    // Greater than or equal comparison operator (>=) for string
    bool string::operator>=(const string& other) const {
        return !(*this < other);
    }

    // Greater than or equal comparison operator (>=) for const char*
    bool string::operator>=(const char* str) const {
        if (!str) return true;
        return !(*this < str);
    }

    // Find methods implementation
    size_t string::find(const char* str, size_t pos) const {
        LOGE("nonstd::string.find(const char*) called with '%s' at pos %zu", str ? str : "NULL", pos);
        
        if (!str || pos >= len) {
            LOGE("nonstd::string.find: invalid parameters, returning npos");
            return npos;
        }
        
        size_t str_len = custom_strlen(str);
        if (str_len == 0) {
            LOGE("nonstd::string.find: empty search string, returning pos %zu", pos);
            return pos;
        }
        
        // Search from pos to len - str_len
        for (size_t i = pos; i <= len - str_len; ++i) {
            bool found = true;
            for (size_t j = 0; j < str_len; ++j) {
                if (data[i + j] != str[j]) {
                    found = false;
                    break;
                }
            }
            if (found) {
                LOGE("nonstd::string.find: found at position %zu", i);
                return i;
            }
        }
        
        LOGE("nonstd::string.find: not found, returning npos");
        return npos;
    }
    
    size_t string::find(const string& str, size_t pos) const {
        LOGE("nonstd::string.find(string) called with '%s' at pos %zu", str.c_str(), pos);
        return find(str.c_str(), pos);
    }
    
    size_t string::find(char ch, size_t pos) const {
        LOGE("nonstd::string.find(char) called with '%c' at pos %zu", ch, pos);
        
        if (pos >= len) {
            LOGE("nonstd::string.find(char): pos >= len, returning npos");
            return npos;
        }
        
        for (size_t i = pos; i < len; ++i) {
            if (data[i] == ch) {
                LOGE("nonstd::string.find(char): found at position %zu", i);
                return i;
            }
        }
        
        LOGE("nonstd::string.find(char): not found, returning npos");
        return npos;
    }
    
    // Reverse find methods implementation (rfind)
    size_t string::rfind(const char* str, size_t pos) const {
        LOGE("nonstd::string.rfind(const char*) called with '%s' at pos %zu", str ? str : "NULL", pos);
        
        if (!str) {
            LOGE("nonstd::string.rfind: str is NULL, returning npos");
            return npos;
        }
        
        size_t str_len = custom_strlen(str);
        if (str_len == 0) {
            LOGE("nonstd::string.rfind: empty search string, returning pos %zu", pos);
            return pos;
        }
        
        // Adjust pos if it's npos or beyond string length
        if (pos == npos || pos >= len) {
            pos = len - 1;
        }
        
        // Search from pos backwards
        for (size_t i = pos; i >= str_len - 1; --i) {
            bool found = true;
            for (size_t j = 0; j < str_len; ++j) {
                if (data[i - str_len + 1 + j] != str[j]) {
                    found = false;
                    break;
                }
            }
            if (found) {
                size_t result = i - str_len + 1;
                LOGE("nonstd::string.rfind: found at position %zu", result);
                return result;
            }
        }
        
        LOGE("nonstd::string.rfind: not found, returning npos");
        return npos;
    }
    
    size_t string::rfind(const string& str, size_t pos) const {
        LOGE("nonstd::string.rfind(string) called with '%s' at pos %zu", str.c_str(), pos);
        return rfind(str.c_str(), pos);
    }
    
    size_t string::rfind(char ch, size_t pos) const {
        LOGE("nonstd::string.rfind(char) called with '%c' at pos %zu", ch, pos);
        
        // Adjust pos if it's npos or beyond string length
        if (pos == npos || pos >= len) {
            pos = len - 1;
        }
        
        for (size_t i = pos; i >= 0; --i) {
            if (data[i] == ch) {
                LOGE("nonstd::string.rfind(char): found at position %zu", i);
                return i;
            }
        }
        
        LOGE("nonstd::string.rfind(char): not found, returning npos");
        return npos;
    }
    
    // Find last of methods implementation (find_last_of)
    size_t string::find_last_of(const char* str, size_t pos) const {
        LOGE("nonstd::string.find_last_of(const char*) called with '%s' at pos %zu", str ? str : "NULL", pos);
        
        if (!str) {
            LOGE("nonstd::string.find_last_of: str is NULL, returning npos");
            return npos;
        }
        
        // Adjust pos if it's npos or beyond string length
        if (pos == npos || pos >= len) {
            pos = len - 1;
        }
        
        // Search from pos backwards for any character in str
        for (size_t i = pos; i >= 0; --i) {
            for (size_t j = 0; str[j] != '\0'; ++j) {
                if (data[i] == str[j]) {
                    LOGE("nonstd::string.find_last_of: found '%c' at position %zu", data[i], i);
                    return i;
                }
            }
        }
        
        LOGE("nonstd::string.find_last_of: not found, returning npos");
        return npos;
    }
    
    size_t string::find_last_of(const string& str, size_t pos) const {
        LOGE("nonstd::string.find_last_of(string) called with '%s' at pos %zu", str.c_str(), pos);
        return find_last_of(str.c_str(), pos);
    }
    
    size_t string::find_last_of(char ch, size_t pos) const {
        LOGE("nonstd::string.find_last_of(char) called with '%c' at pos %zu", ch, pos);
        
        // Adjust pos if it's npos or beyond string length
        if (pos == npos || pos >= len) {
            pos = len - 1;
        }
        
        // Search from pos backwards for the character
        for (size_t i = pos; i >= 0; --i) {
            if (data[i] == ch) {
                LOGE("nonstd::string.find_last_of(char): found '%c' at position %zu", ch, i);
                return i;
            }
        }
        
        LOGE("nonstd::string.find_last_of(char): not found, returning npos");
        return npos;
    }
    
    // Compare methods implementation
    int string::compare(const string& str) const {
        LOGE("nonstd::string.compare(string) called, comparing '%s' with '%s'", data, str.c_str());
        
        size_t min_len = (len < str.len) ? len : str.len;
        
        for (size_t i = 0; i < min_len; ++i) {
            if (data[i] < str.data[i]) {
                LOGE("nonstd::string.compare: '%c' < '%c' at position %zu, returning -1", data[i], str.data[i], i);
                return -1;
            } else if (data[i] > str.data[i]) {
                LOGE("nonstd::string.compare: '%c' > '%c' at position %zu, returning 1", data[i], str.data[i], i);
                return 1;
            }
        }
        
        // If all characters are equal up to min_len, compare lengths
        if (len < str.len) {
            LOGE("nonstd::string.compare: lengths compared (%zu < %zu), returning -1", len, str.len);
            return -1;
        } else if (len > str.len) {
            LOGE("nonstd::string.compare: lengths compared (%zu > %zu), returning 1", len, str.len);
            return 1;
        } else {
            LOGE("nonstd::string.compare: strings are equal, returning 0");
            return 0;
        }
    }
    
    int string::compare(const char* str) const {
        LOGE("nonstd::string.compare(const char*) called, comparing '%s' with '%s'", data, str ? str : "NULL");
        
        if (!str) {
            LOGE("nonstd::string.compare: str is NULL, returning 1");
            return 1;
        }
        
        size_t str_len = custom_strlen(str);
        size_t min_len = (len < str_len) ? len : str_len;
        
        for (size_t i = 0; i < min_len; ++i) {
            if (data[i] < str[i]) {
                LOGE("nonstd::string.compare: '%c' < '%c' at position %zu, returning -1", data[i], str[i], i);
                return -1;
            } else if (data[i] > str[i]) {
                LOGE("nonstd::string.compare: '%c' > '%c' at position %zu, returning 1", data[i], str[i], i);
                return 1;
            }
        }
        
        // If all characters are equal up to min_len, compare lengths
        if (len < str_len) {
            LOGE("nonstd::string.compare: lengths compared (%zu < %zu), returning -1", len, str_len);
            return -1;
        } else if (len > str_len) {
            LOGE("nonstd::string.compare: lengths compared (%zu > %zu), returning 1", len, str_len);
            return 1;
        } else {
            LOGE("nonstd::string.compare: strings are equal, returning 0");
            return 0;
        }
    }
    
    int string::compare(size_t pos, size_t len, const string& str) const {
        LOGE("nonstd::string.compare(pos=%zu, len=%zu, string) called", pos, len);
        
        if (pos >= this->len) {
            LOGE("nonstd::string.compare: pos >= length, returning -1");
            return -1;
        }
        
        // Adjust len if it would exceed the string length
        if (pos + len > this->len) {
            len = this->len - pos;
        }
        
        // Create a temporary string for the substring comparison
        string temp_str(data + pos, len);
        return temp_str.compare(str);
    }
    
    int string::compare(size_t pos, size_t len, const char* str) const {
        LOGE("nonstd::string.compare(pos=%zu, len=%zu, const char*) called", pos, len);
        
        if (pos >= this->len) {
            LOGE("nonstd::string.compare: pos >= length, returning -1");
            return -1;
        }
        
        // Adjust len if it would exceed the string length
        if (pos + len > this->len) {
            len = this->len - pos;
        }
        
        // Create a temporary string for the substring comparison
        string temp_str(data + pos, len);
        return temp_str.compare(str);
    }
    
    int string::compare(size_t pos, size_t len, const string& str, size_t subpos, size_t sublen) const {
        LOGE("nonstd::string.compare(pos=%zu, len=%zu, string, subpos=%zu, sublen=%zu) called", pos, len, subpos, sublen);
        
        if (pos >= this->len || subpos >= str.len) {
            LOGE("nonstd::string.compare: invalid positions, returning -1");
            return -1;
        }
        
        // Adjust lengths if they would exceed the string lengths
        if (pos + len > this->len) {
            len = this->len - pos;
        }
        if (subpos + sublen > str.len) {
            sublen = str.len - subpos;
        }
        
        // Compare the substrings
        size_t min_len = (len < sublen) ? len : sublen;
        
        for (size_t i = 0; i < min_len; ++i) {
            if (data[pos + i] < str.data[subpos + i]) {
                LOGE("nonstd::string.compare: '%c' < '%c' at position %zu, returning -1", data[pos + i], str.data[subpos + i], i);
                return -1;
            } else if (data[pos + i] > str.data[subpos + i]) {
                LOGE("nonstd::string.compare: '%c' > '%c' at position %zu, returning 1", data[pos + i], str.data[subpos + i], i);
                return 1;
            }
        }
        
        // If all characters are equal up to min_len, compare lengths
        if (len < sublen) {
            LOGE("nonstd::string.compare: substring lengths compared (%zu < %zu), returning -1", len, sublen);
            return -1;
        } else if (len > sublen) {
            LOGE("nonstd::string.compare: substring lengths compared (%zu > %zu), returning 1", len, sublen);
            return 1;
        } else {
            LOGE("nonstd::string.compare: substrings are equal, returning 0");
            return 0;
        }
    }
    
    int string::compare(size_t pos, size_t len, const char* str, size_t sublen) const {
        LOGE("nonstd::string.compare(pos=%zu, len=%zu, const char*, sublen=%zu) called", pos, len, sublen);
        
        if (pos >= this->len || !str) {
            LOGE("nonstd::string.compare: invalid parameters, returning -1");
            return -1;
        }
        
        // Adjust len if it would exceed the string length
        if (pos + len > this->len) {
            len = this->len - pos;
        }
        
        // Compare the substrings
        size_t min_len = (len < sublen) ? len : sublen;
        
        for (size_t i = 0; i < min_len; ++i) {
            if (data[pos + i] < str[i]) {
                LOGE("nonstd::string.compare: '%c' < '%c' at position %zu, returning -1", data[pos + i], str[i], i);
                return -1;
            } else if (data[pos + i] > str[i]) {
                LOGE("nonstd::string.compare: '%c' > '%c' at position %zu, returning 1", data[pos + i], str[i], i);
                return 1;
            }
        }
        
        // If all characters are equal up to min_len, compare lengths
        if (len < sublen) {
            LOGE("nonstd::string.compare: substring lengths compared (%zu < %zu), returning -1", len, sublen);
            return -1;
        } else if (len > sublen) {
            LOGE("nonstd::string.compare: substring lengths compared (%zu > %zu), returning 1", len, sublen);
            return 1;
        } else {
            LOGE("nonstd::string.compare: substrings are equal, returning 0");
            return 0;
        }
    }

} // namespace nonstd

