#ifndef zString_H
#define zString_H

#include "zConfig.h"
#include "zLog.h"

namespace nonstd {
#if ZCONFIG_ENABLE_NONSTD_API
    class string {
    private:
        char* data;  // Pointer to dynamic char array (C-string)
        size_t len;  // Length of the string
        size_t capacity;

        // Custom implementation of strlen (get the length of the string)
        size_t custom_strlen(const char* str) const;

        // Custom implementation of strcpy (copy a C-string)
        char* custom_strcpy(char* dest, const char* src) const;

        // Custom implementation of strcat (concatenate two C-strings)
        char* custom_strcat(char* dest, const char* src) const;

    public:

        // Static constant for maximum size
        static const size_t npos = -1;

        // Default constructor
        string();

        // Constructor from C-string
        string(const char* str);

        // Constructor from C-string with length
        string(const char* str, size_t len);

        // Constructor from iterator range
        template<typename InputIt>
        string(InputIt first, InputIt last) {
            // Calculate the size of the range
            size_t count = 0;
            for (InputIt it = first; it != last; ++it) {
                ++count;
            }
            
            // Allocate memory
            capacity = count + 1;
            data = new char[capacity];
            len = count;
            
            // Copy elements
            size_t i = 0;
            for (InputIt it = first; it != last; ++it, ++i) {
                data[i] = static_cast<char>(*it);
            }
            data[len] = '\0';
        }

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
        
        // Get the size of the string (same as length)
        size_t size() const;

        // Check if the string is empty
        bool empty() const;

        // Clear the string
        void clear();

        // Get substring starting at pos with length len
        string substr(size_t pos = 0, size_t len = npos) const;

        // Access the underlying C-string
        const char* c_str() const;



        // Iterators for range-based for loop support
        class iterator {
        private:
            char* ptr;
        public:
            iterator(char* p) : ptr(p) {}
            char& operator*() { return *ptr; }
            iterator& operator++() { ++ptr; return *this; }
            iterator operator++(int) { iterator temp = *this; ++ptr; return temp; }
            bool operator==(const iterator& other) const { return ptr == other.ptr; }
            bool operator!=(const iterator& other) const { return ptr != other.ptr; }
        };

        class const_iterator {
        private:
            const char* ptr;
        public:
            const_iterator(const char* p) : ptr(p) {}
            const char& operator*() const { return *ptr; }
            const_iterator& operator++() { ++ptr; return *this; }
            const_iterator operator++(int) { const_iterator temp = *this; ++ptr; return temp; }
            bool operator==(const const_iterator& other) const { return ptr == other.ptr; }
            bool operator!=(const const_iterator& other) const { return ptr != other.ptr; }
        };

        iterator begin() { return iterator(data); }
        iterator end() { return iterator(data + len); }
        const_iterator begin() const { return const_iterator(data); }
        const_iterator end() const { return const_iterator(data + len); }

        // Operator[] for character access
        char& operator[](size_t index);

        // Const version of operator[] for read-only access
        const char& operator[](size_t index) const;

        // Concatenation operator (A + B)
        string operator+(const string& other) const;
        string operator+(char ch) const;
        
        // Friend function for const char* + string
        friend string operator+(const char* str, const string& other);
        // Friend function for char + string
        friend string operator+(char ch, const string& str);
        
        // Compound assignment operator (+=)
        string& operator+=(const string& other);
        string& operator+=(const char* str);
        string& operator+=(char ch);
        
        // Equality comparison operators
        bool operator==(const string& other) const;
        bool operator==(const char* str) const;
        
        // Inequality comparison operators
        bool operator!=(const string& other) const;
        bool operator!=(const char* str) const;
        
        // Less than comparison operators
        bool operator<(const string& other) const;
        bool operator<(const char* str) const;
        
        // Greater than comparison operators
        bool operator>(const string& other) const;
        bool operator>(const char* str) const;
        
        // Less than or equal comparison operators
        bool operator<=(const string& other) const;
        bool operator<=(const char* str) const;
        
        // Greater than or equal comparison operators
        bool operator>=(const string& other) const;
        bool operator>=(const char* str) const;
        
        // Find methods
        size_t find(const char* str, size_t pos = 0) const;
        size_t find(const string& str, size_t pos = 0) const;
        size_t find(char ch, size_t pos = 0) const;
        
        // Reverse find methods (rfind)
        size_t rfind(const char* str, size_t pos = npos) const;
        size_t rfind(const string& str, size_t pos = npos) const;
        size_t rfind(char ch, size_t pos = npos) const;

        size_t find_first_not_of(const char* s, size_t pos) const;

        // Find last of methods (find_last_of)
        size_t find_last_of(const char* str, size_t pos = npos) const;
        size_t find_last_of(char ch, size_t pos = npos) const;
        
        // Compare methods
        int compare(const string& str) const;
        int compare(const char* str) const;
        int compare(size_t pos, size_t len, const string& str) const;
        int compare(size_t pos, size_t len, const char* str) const;
        int compare(size_t pos, size_t len, const string& str, size_t subpos, size_t sublen) const;
        int compare(size_t pos, size_t len, const char* str, size_t sublen) const;

        //
        const char& back() const;

        // 删除字符串最后一个字符
        void pop_back();


        void reserve(size_t new_cap);

        string& append(const char* s, size_t n);

        string& append(const char* s);

    };

    string to_string(int value);
#endif
} // namespace nonstd

#endif // zString_H
