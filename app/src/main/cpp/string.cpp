// string.cpp
#include "string.h"
#include <cstring>  // For custom_strlen, custom_strcpy, and custom_strcat implementations
#include <stdexcept>  // For std::out_of_range

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

// Access the underlying C-string
const char* string::c_str() const {
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

} // namespace nonstd

