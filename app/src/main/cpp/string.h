// string.h
#ifndef zString_H
#define zString_H

#include <stdexcept>  // For std::out_of_range

namespace nonstd {

class string {
private:
    char* data;  // Pointer to dynamic char array (C-string)
    size_t len;  // Length of the string

    // Custom implementation of strlen (get the length of the string)
    size_t custom_strlen(const char* str) const;

    // Custom implementation of strcpy (copy a C-string)
    char* custom_strcpy(char* dest, const char* src) const;

    // Custom implementation of strcat (concatenate two C-strings)
    char* custom_strcat(char* dest, const char* src) const;

public:
    // Default constructor
    string();

    // Constructor from C-string
    string(const char* str);

    // Copy constructor
    string(const string& other);

    // Move constructor
    string(string&& other) noexcept;

    // Destructor
    ~string();

    // Copy assignment operator
    string& operator=(const string& other);

    // Move assignment operator
    string& operator=(string&& other) noexcept;

    // Get the length of the string
    size_t length() const;

    // Access the underlying C-string
    const char* c_str() const;

    // Operator[] for character access
    char& operator[](size_t index);

    // Const version of operator[] for read-only access
    const char& operator[](size_t index) const;

    // Concatenation operator (A + B)
    string operator+(const string& other) const;
};

} // namespace nonstd

#endif // zString_H
