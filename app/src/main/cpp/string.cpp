// string.cpp

#include <cstring>  // For custom_strlen, custom_strcpy, and custom_strcat implementations
#include <stdexcept>  // For std::out_of_range
#include "string.h"

namespace nonstd {

    /**
     * 从C字符串构造string对象
     * 根据C风格字符串创建string对象，自动计算长度
     * @param str C风格字符串指针
     */
    string::string(const char* str) {
        LOGV("nonstd::string(const char* str) is called with str=%p", str);
        if (!str) {
            LOGV("nonstd::string: str is NULL, creating empty string");
            data = new char[1]{'\0'};
            len = 0;
            capacity = 1;
            return;
        }
        len = custom_strlen(str);  // 使用自定义strlen计算长度
        data = new char[len + 1];  // +1为null终止符
        capacity = len + 1;
        custom_strcpy(data, str);  // 使用自定义strcpy复制字符串
        LOGV("nonstd::string(const char*) completed, data=%p, length=%zu", data, len);
    }

    /**
     * 自定义strlen实现
     * 计算C风格字符串的长度，不包含null终止符
     * @param str 要计算长度的字符串
     * @return 字符串长度
     */
    size_t string::custom_strlen(const char* str) const {
        if (!str) return 0;
        size_t length = 0;
        while (str[length] != '\0') {
            ++length;
        }
        return length;
    }

    /**
     * 自定义strcpy实现
     * 将源字符串复制到目标缓冲区
     * @param dest 目标缓冲区
     * @param src 源字符串
     * @return 目标缓冲区指针
     */
    char* string::custom_strcpy(char* dest, const char* src) const {
        if (!dest || !src) return dest;
        char* d = dest;
        while ((*d++ = *src++) != '\0') {}  // 逐字符复制
        return dest;
    }

    /**
     * 自定义strcat实现
     * 将源字符串追加到目标字符串末尾
     * @param dest 目标字符串
     * @param src 要追加的源字符串
     * @return 目标字符串指针
     */
    char* string::custom_strcat(char* dest, const char* src) const {
        if (!dest || !src) return dest;
        char* d = dest;
        while (*d != '\0') {  // 找到第一个字符串的末尾
            ++d;
        }
        while ((*d++ = *src++) != '\0') {}  // 追加第二个字符串
        return dest;
    }

    /**
     * 默认构造函数
     * 创建空的string对象
     */
    string::string() : data(new char[1]{'\0'}), len(0), capacity(1) {
        LOGV("nonstd::string() default constructor called, data=%p", data);
    }

    /**
     * 从C字符串和长度构造string对象
     * 根据C风格字符串和指定长度创建string对象
     * @param str C风格字符串指针
     * @param len 要复制的字符长度
     */
    string::string(const char* str, size_t len) {
        LOGV("nonstd::string(const char* str, size_t len) is called with str=%p, len=%zu", str, len);
        
        // 遵循std::string行为：如果len为0则允许nullptr
        if (!str && len > 0) {
            LOGV("nonstd::string: str is NULL but len > 0, throwing logic_error");
            throw std::logic_error("nonstd::string: construction from null is not valid");
        }
        
        this->len = len;
        if (len > 0) {
            data = new char[len + 1];
            capacity = len + 1;
            // 复制指定长度的字符
            for (size_t i = 0; i < len; ++i) {
                data[i] = str[i];
            }
            data[len] = '\0';
        } else {
            // 空字符串
            data = new char[1]{'\0'};
            capacity = 1;
        }
        
        LOGV("nonstd::string(const char*, size_t) completed, data=%p, length=%zu", data, this->len);
    }

    /**
     * 拷贝构造函数
     * 根据另一个string对象创建新的string对象
     * @param other 要拷贝的string对象
     */
    string::string(const string& other) {
        LOGV("nonstd::string(const string& other) copy constructor called, other.data=%p, other.len=%zu", other.data, other.len);
        len = other.len;
        data = new char[len + 1];
        capacity = len + 1;
        if (other.data) {
            custom_strcpy(data, other.data);  // 使用自定义strcpy
        } else {
            data[0] = '\0';
        }
        LOGV("nonstd::string(const string&) completed, data=%p, length=%zu", data, len);
    }

    /**
     * 移动构造函数
     * 从另一个string对象移动资源，原对象变为空状态
     * @param other 要移动的string对象
     */
    string::string(string&& other) noexcept : data(other.data), len(other.len), capacity(other.capacity) {
        LOGV("nonstd::string(string&& other) move constructor called, other.data=%p, other.len=%zu", other.data, other.len);
        other.data = nullptr;  // 将原对象的数据指针置空
        other.len = 0;
        other.capacity = 0;
        LOGV("nonstd::string(string&&) completed, data=%p, length=%zu", data, len);
    }

    /**
     * 析构函数
     * 释放字符串占用的内存
     */
    string::~string() {
        LOGV("nonstd::string destructor called, data=%p, len=%zu", data, len);
        if (data) {
            delete[] data;
        }
    }

    /**
     * 拷贝赋值操作符
     * 将另一个string对象的内容拷贝到当前对象
     * @param other 要拷贝的string对象
     * @return 当前对象的引用
     */
    string& string::operator=(const string& other) {
        LOGV("nonstd::string.operator=(const string& other) called, other.data=%p, other.len=%zu", other.data, other.len);
        if (this == &other) return *this;  // 自赋值检查

        if (data) {
            delete[] data;  // 清理现有数据
        }

        len = other.len;
        data = new char[len + 1];
        capacity = len + 1;
        if (other.data) {
            custom_strcpy(data, other.data);  
        } else {
            data[0] = '\0';
        }

        LOGV("nonstd::string.operator= completed, data=%p, length=%zu", data, len);
        return *this;
    }

    /**
     * 移动赋值操作符
     * 从另一个string对象移动资源到当前对象
     * @param other 要移动的string对象
     * @return 当前对象的引用
     */
    string& string::operator=(string&& other) noexcept {
        LOGV("nonstd::string.operator=(string&& other) move assignment called, other.data=%p, other.len=%zu", other.data, other.len);
        if (this != &other) {
            if (data) {
                delete[] data;  // 清理现有数据
            }

            data = other.data;
            len = other.len;
            capacity = other.capacity;

            other.data = nullptr;  // 将原对象的数据指针置空
            other.len = 0;
            other.capacity = 0;
        }
        LOGV("nonstd::string.operator=(string&&) completed, data=%p, length=%zu", data, len);
        return *this;
    }

    /**
     * 获取字符串长度
     * @return 字符串的字符数量
     */
    size_t string::length() const {
        return len;
    }

    /**
     * 获取字符串大小
     * 与length()函数相同
     * @return 字符串的字符数量
     */
    size_t string::size() const {
        LOGV("nonstd::string.size() is called, returning %zu", len);
        return len;
    }

    /**
     * 检查字符串是否为空
     * @return 如果字符串为空返回true
     */
    bool string::empty() const {
        LOGV("nonstd::string.empty() is called, returning %s", (len == 0) ? "true" : "false");
        return len == 0;
    }

    /**
     * 获取C风格字符串指针
     * @return 指向内部字符数组的指针
     */
    const char* string::c_str() const {
        LOGV("nonstd::string.c_str() is called, data=%p, len=%zu", data, len);
        if (!data) {
            LOGV("nonstd::string.c_str(): data is NULL, returning empty string");
            return "";
        }
        return data;
    }

    /**
     * 清空字符串内容
     * 将字符串长度设为0，但保留内存分配
     */
    void string::clear() {
        LOGV("nonstd::string.clear() is called, old data=%p, old length=%zu", data, len);
        if (data) {
            data[0] = '\0';
        }
        len = 0;
    }

    /**
     * 获取子字符串
     * 从指定位置开始提取指定长度的子字符串
     * @param pos 起始位置
     * @param len 子字符串长度
     * @return 新的string对象，包含子字符串
     */
    string string::substr(size_t pos, size_t len) const {
        LOGV("nonstd::string.substr(pos=%zu, len=%zu) is called", pos, len);
        
        // Check bounds
        if (pos >= this->len) {
            LOGV("nonstd::string.substr: pos %zu >= length %zu, throwing out_of_range", pos, this->len);
            throw std::out_of_range("string::substr");
        }
        
        // Calculate actual length
        size_t actual_len = len;
        if (len == npos || pos + len > this->len) {
            actual_len = this->len - pos;
        }
        
        LOGV("nonstd::string.substr: creating substring of length %zu", actual_len);
        
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
        
        LOGV("nonstd::string.substr: returning substring '%s'", result.c_str());
        return result;
    }

    /**
     * 下标操作符（非const版本）
     * 返回指定位置的字符引用，允许修改
     * @param index 字符位置
     * @return 字符的引用
     */
    char& string::operator[](size_t index) {
        if (index >= len) {
            throw std::out_of_range("Index out of range");
        }
        return data[index];
    }

    /**
     * 下标操作符（const版本）
     * 返回指定位置的字符引用，不允许修改
     * @param index 字符位置
     * @return 字符的const引用
     */
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
        result.capacity = result.len + 1;

        custom_strcpy(result.data, data);  // Copy this string
        custom_strcat(result.data, other.data);  // Concatenate the other string

        return result;
    }

// Friend function for const char* + string
    string operator+(const char* str, const string& other) {
        LOGV("nonstd::operator+(const char*, const string&) called, str='%s', other='%s'", str, other.c_str());
        
        size_t str_len = 0;
        while (str[str_len] != '\0') {
            ++str_len;
        }
        
        string result;
        result.len = str_len + other.len;
        result.data = new char[result.len + 1];  // +1 for null terminator
        result.capacity = result.len + 1;
        
        // Copy the C-string first
        char* dest = result.data;
        const char* src = str;
        while ((*dest++ = *src++) != '\0') {}
        --dest; // Move back one position (over the null terminator)
        
        // Then copy the string
        src = other.data;
        while ((*dest++ = *src++) != '\0') {}
        
        LOGV("nonstd::operator+(const char*, const string&) done, result='%s'", result.c_str());
        return result;
    }

// Compound assignment operator (+=) for string
    string& string::operator+=(const string& other) {
        LOGV("nonstd::string.operator+=(const string&) called, this len=%zu, other len=%zu", len, other.len);
        
        size_t new_len = len + other.len;
        char* new_data = new char[new_len + 1];  // +1 for null terminator
        
        custom_strcpy(new_data, data);  // Copy this string
        custom_strcat(new_data, other.data);  // Concatenate the other string
        
        delete[] data;  // Clean up old data
        data = new_data;
        len = new_len;
        capacity = new_len + 1;
        
        LOGV("nonstd::string.operator+=(const string&) done, new len=%zu", len);
        return *this;
    }

// Compound assignment operator (+=) for C-string
    string& string::operator+=(const char* str) {
        LOGV("nonstd::string.operator+=(const char*) called, this len=%zu, str len=%zu", len, custom_strlen(str));
        
        size_t str_len = custom_strlen(str);
        size_t new_len = len + str_len;
        char* new_data = new char[new_len + 1];  // +1 for null terminator
        
        custom_strcpy(new_data, data);  // Copy this string
        custom_strcat(new_data, str);   // Concatenate the C-string
        
        delete[] data;  // Clean up old data
        data = new_data;
        len = new_len;
        capacity = new_len + 1;
        
        LOGV("nonstd::string.operator+=(const char*) done, new len=%zu", len);
        return *this;
    }

// Compound assignment operator (+=) for char
    string& string::operator+=(char ch) {
        LOGV("nonstd::string.operator+=(char '%c') is called, current length=%zu", ch, len);
        
        size_t new_len = len + 1;  // Add one character
        char* new_data = new char[new_len + 1];  // +1 for null terminator
        
        custom_strcpy(new_data, data);  // Copy this string
        new_data[len] = ch;             // Append the character
        new_data[new_len] = '\0';       // Add null terminator
        
        delete[] data;  // Clean up old data
        data = new_data;
        len = new_len;
        capacity = new_len + 1;
        
        LOGV("nonstd::string.operator+=(char) completed, new length=%zu", len);
        return *this;
    }

    // Equality comparison operator (==) for string
    bool string::operator==(const string& other) const {
        LOGV("nonstd::string.operator==(string) is called, comparing '%s' with '%s'", data, other.data);
        
        if (len != other.len) {
            LOGV("nonstd::string.operator==(string): lengths differ (%zu vs %zu), returning false", len, other.len);
            return false;
        }
        
        // Compare each character
        for (size_t i = 0; i < len; ++i) {
            if (data[i] != other.data[i]) {
                LOGV("nonstd::string.operator==(string): characters differ at position %zu ('%c' vs '%c'), returning false", i, data[i], other.data[i]);
                return false;
            }
        }
        
        LOGV("nonstd::string.operator==(string): strings are equal, returning true");
        return true;
    }

    // Equality comparison operator (==) for const char*
    bool string::operator==(const char* str) const {
        LOGV("nonstd::string.operator==(const char*) is called, comparing '%s' with '%s'", data, str ? str : "NULL");
        
        if (!str) {
            LOGV("nonstd::string.operator==(const char*): str is NULL, returning false");
            return false;
        }
        
        // Compare each character
        size_t i = 0;
        while (i < len && str[i] != '\0') {
            if (data[i] != str[i]) {
                LOGV("nonstd::string.operator==(const char*): characters differ at position %zu ('%c' vs '%c'), returning false", i, data[i], str[i]);
                return false;
            }
            i++;
        }
        
        // Check if both strings ended at the same time
        bool result = (i == len && str[i] == '\0');
        LOGV("nonstd::string.operator==(const char*): comparison result = %s", result ? "true" : "false");
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
        LOGV("nonstd::string.operator<(string) is called, comparing '%s' with '%s'", data, other.data);
        
        size_t min_len = (len < other.len) ? len : other.len;
        
        for (size_t i = 0; i < min_len; ++i) {
            if (data[i] < other.data[i]) {
                LOGV("nonstd::string.operator<(string): '%c' < '%c' at position %zu, returning true", data[i], other.data[i], i);
                return true;
            } else if (data[i] > other.data[i]) {
                LOGV("nonstd::string.operator<(string): '%c' > '%c' at position %zu, returning false", data[i], other.data[i], i);
                return false;
            }
        }
        
        // If all characters are equal up to min_len, shorter string is less
        bool result = (len < other.len);
        LOGV("nonstd::string.operator<(string): lengths compared (%zu < %zu), returning %s", len, other.len, result ? "true" : "false");
        return result;
    }

    // Less than comparison operator (<) for const char*
    bool string::operator<(const char* str) const {
        LOGV("nonstd::string.operator<(const char*) is called, comparing '%s' with '%s'", data, str ? str : "NULL");
        
        if (!str) {
            LOGV("nonstd::string.operator<(const char*): str is NULL, returning false");
            return false;
        }
        
        size_t i = 0;
        while (i < len && str[i] != '\0') {
            if (data[i] < str[i]) {
                LOGV("nonstd::string.operator<(const char*): '%c' < '%c' at position %zu, returning true", data[i], str[i], i);
                return true;
            } else if (data[i] > str[i]) {
                LOGV("nonstd::string.operator<(const char*): '%c' > '%c' at position %zu, returning false", data[i], str[i], i);
                return false;
            }
            i++;
        }
        
        // If all characters are equal up to min_len, shorter string is less
        bool result = (i == len && str[i] != '\0');
        LOGV("nonstd::string.operator<(const char*): comparison result = %s", result ? "true" : "false");
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
        LOGV("nonstd::string.find(const char*) called with '%s' at pos %zu", str ? str : "NULL", pos);
        
        if (!str || pos >= len) {
            LOGV("nonstd::string.find: invalid parameters, returning npos");
            return npos;
        }
        
        size_t str_len = custom_strlen(str);
        if (str_len == 0) {
            LOGV("nonstd::string.find: empty search string, returning pos %zu", pos);
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
                LOGV("nonstd::string.find: found at position %zu", i);
                return i;
            }
        }
        
        LOGV("nonstd::string.find: not found, returning npos");
        return npos;
    }
    
    size_t string::find(const string& str, size_t pos) const {
        LOGV("nonstd::string.find(string) called with '%s' at pos %zu", str.c_str(), pos);
        return find(str.c_str(), pos);
    }
    
    size_t string::find(char ch, size_t pos) const {
        LOGV("nonstd::string.find(char) called with '%c' at pos %zu", ch, pos);
        
        if (pos >= len) {
            LOGV("nonstd::string.find(char): pos >= len, returning npos");
            return npos;
        }
        
        for (size_t i = pos; i < len; ++i) {
            if (data[i] == ch) {
                LOGV("nonstd::string.find(char): found at position %zu", i);
                return i;
            }
        }
        
        LOGV("nonstd::string.find(char): not found, returning npos");
        return npos;
    }

    size_t string::find_first_not_of(const char* s, size_t pos) const {
        LOGV("nonstd::string.find_first_not_of(const char*) called with \"%s\" at pos %zu", s, pos);

        if (!s) {
            LOGV("nonstd::string.find_first_not_of: null pointer passed, returning npos");
            return npos;
        }

        if (pos >= len) {
            LOGV("nonstd::string.find_first_not_of: pos >= len, returning npos");
            return npos;
        }

        for (size_t i = pos; i < len; ++i) {
            bool found = false;
            for (size_t j = 0; s[j] != '\0'; ++j) {
                if (data[i] == s[j]) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                LOGV("nonstd::string.find_first_not_of: first non-matching character at position %zu", i);
                return i;
            }
        }

        LOGV("nonstd::string.find_first_not_of: all characters matched, returning npos");
        return npos;
    }

    
    // Reverse find methods implementation (rfind)
    size_t string::rfind(const char* str, size_t pos) const {
        LOGV("nonstd::string.rfind(const char*) called with '%s' at pos %zu", str ? str : "NULL", pos);
        
        if (!str) {
            LOGV("nonstd::string.rfind: str is NULL, returning npos");
            return npos;
        }
        
        size_t str_len = custom_strlen(str);
        if (str_len == 0) {
            LOGV("nonstd::string.rfind: empty search string, returning pos %zu", pos);
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
                LOGV("nonstd::string.rfind: found at position %zu", result);
                return result;
            }
        }
        
        LOGV("nonstd::string.rfind: not found, returning npos");
        return npos;
    }
    
    size_t string::rfind(const string& str, size_t pos) const {
        LOGV("nonstd::string.rfind(string) called with '%s' at pos %zu", str.c_str(), pos);
        return rfind(str.c_str(), pos);
    }
    
    size_t string::rfind(char ch, size_t pos) const {
        LOGV("nonstd::string.rfind(char) called with '%c' at pos %zu", ch, pos);
        
        // Adjust pos if it's npos or beyond string length
        if (pos == npos || pos >= len) {
            pos = len - 1;
        }
        
        // Search from pos backwards for the character
        for (int i = static_cast<int>(pos); i >= 0; --i) {
            if (data[i] == ch) {
                LOGV("nonstd::string.rfind(char): found at position %zu", static_cast<size_t>(i));
                return static_cast<size_t>(i);
            }
        }
        
        LOGV("nonstd::string.rfind(char): not found, returning npos");
        return npos;
    }

    size_t string::find_last_of(const string& str, size_t pos) const {
        LOGV("nonstd::string.find_last_of(string) called with '%s' at pos %zu", str.c_str(), pos);
        return find_last_of(str.c_str(), pos);
    }
    
    size_t string::find_last_of(char ch, size_t pos) const {
        LOGV("nonstd::string.find_last_of(char) called with '%c' at pos %zu", ch, pos);
        
        // Adjust pos if it's npos or beyond string length
        if (pos == npos || pos >= len) {
            pos = len - 1;
        }
        
        // Search from pos backwards for the character
        for (int i = static_cast<int>(pos); i >= 0; --i) {
            if (data[i] == ch) {
                LOGV("nonstd::string.find_last_of(char): found '%c' at position %zu", ch, static_cast<size_t>(i));
                return static_cast<size_t>(i);
            }
        }
        
        LOGV("nonstd::string.find_last_of(char): not found, returning npos");
        return npos;
    }
    
    // Compare methods implementation
    int string::compare(const string& str) const {
        LOGV("nonstd::string.compare(string) called, comparing '%s' with '%s'", data, str.c_str());
        
        size_t min_len = (len < str.len) ? len : str.len;
        
        for (size_t i = 0; i < min_len; ++i) {
            if (data[i] < str.data[i]) {
                LOGV("nonstd::string.compare: '%c' < '%c' at position %zu, returning -1", data[i], str.data[i], i);
                return -1;
            } else if (data[i] > str.data[i]) {
                LOGV("nonstd::string.compare: '%c' > '%c' at position %zu, returning 1", data[i], str.data[i], i);
                return 1;
            }
        }
        
        // If all characters are equal up to min_len, compare lengths
        if (len < str.len) {
            LOGV("nonstd::string.compare: lengths compared (%zu < %zu), returning -1", len, str.len);
            return -1;
        } else if (len > str.len) {
            LOGV("nonstd::string.compare: lengths compared (%zu > %zu), returning 1", len, str.len);
            return 1;
        } else {
            LOGV("nonstd::string.compare: strings are equal, returning 0");
            return 0;
        }
    }
    
    int string::compare(const char* str) const {
        LOGV("nonstd::string.compare(const char*) called, comparing '%s' with '%s'", data, str ? str : "NULL");
        
        if (!str) {
            LOGV("nonstd::string.compare: str is NULL, returning 1");
            return 1;
        }
        
        size_t str_len = custom_strlen(str);
        size_t min_len = (len < str_len) ? len : str_len;
        
        for (size_t i = 0; i < min_len; ++i) {
            if (data[i] < str[i]) {
                LOGV("nonstd::string.compare: '%c' < '%c' at position %zu, returning -1", data[i], str[i], i);
                return -1;
            } else if (data[i] > str[i]) {
                LOGV("nonstd::string.compare: '%c' > '%c' at position %zu, returning 1", data[i], str[i], i);
                return 1;
            }
        }
        
        // If all characters are equal up to min_len, compare lengths
        if (len < str_len) {
            LOGV("nonstd::string.compare: lengths compared (%zu < %zu), returning -1", len, str_len);
            return -1;
        } else if (len > str_len) {
            LOGV("nonstd::string.compare: lengths compared (%zu > %zu), returning 1", len, str_len);
            return 1;
        } else {
            LOGV("nonstd::string.compare: strings are equal, returning 0");
            return 0;
        }
    }
    
    int string::compare(size_t pos, size_t len, const string& str) const {
        LOGV("nonstd::string.compare(pos=%zu, len=%zu, string) called", pos, len);
        
        if (pos >= this->len) {
            LOGV("nonstd::string.compare: pos >= length, returning -1");
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
        LOGV("nonstd::string.compare(pos=%zu, len=%zu, const char*) called", pos, len);
        
        if (pos >= this->len) {
            LOGV("nonstd::string.compare: pos >= length, returning -1");
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
        LOGV("nonstd::string.compare(pos=%zu, len=%zu, string, subpos=%zu, sublen=%zu) called", pos, len, subpos, sublen);
        
        if (pos >= this->len || subpos >= str.len) {
            LOGV("nonstd::string.compare: invalid positions, returning -1");
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
                LOGV("nonstd::string.compare: '%c' < '%c' at position %zu, returning -1", data[pos + i], str.data[subpos + i], i);
                return -1;
            } else if (data[pos + i] > str.data[subpos + i]) {
                LOGV("nonstd::string.compare: '%c' > '%c' at position %zu, returning 1", data[pos + i], str.data[subpos + i], i);
                return 1;
            }
        }
        
        // If all characters are equal up to min_len, compare lengths
        if (len < sublen) {
            LOGV("nonstd::string.compare: substring lengths compared (%zu < %zu), returning -1", len, sublen);
            return -1;
        } else if (len > sublen) {
            LOGV("nonstd::string.compare: substring lengths compared (%zu > %zu), returning 1", len, sublen);
            return 1;
        } else {
            LOGV("nonstd::string.compare: substrings are equal, returning 0");
            return 0;
        }
    }
    
    int string::compare(size_t pos, size_t len, const char* str, size_t sublen) const {
        LOGV("nonstd::string.compare(pos=%zu, len=%zu, const char*, sublen=%zu) called", pos, len, sublen);
        
        if (pos >= this->len || !str) {
            LOGV("nonstd::string.compare: invalid parameters, returning -1");
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
                LOGV("nonstd::string.compare: '%c' < '%c' at position %zu, returning -1", data[pos + i], str[i], i);
                return -1;
            } else if (data[pos + i] > str[i]) {
                LOGV("nonstd::string.compare: '%c' > '%c' at position %zu, returning 1", data[pos + i], str[i], i);
                return 1;
            }
        }
        
        // If all characters are equal up to min_len, compare lengths
        if (len < sublen) {
            LOGV("nonstd::string.compare: substring lengths compared (%zu < %zu), returning -1", len, sublen);
            return -1;
        } else if (len > sublen) {
            LOGV("nonstd::string.compare: substring lengths compared (%zu > %zu), returning 1", len, sublen);
            return 1;
        } else {
            LOGV("nonstd::string.compare: substrings are equal, returning 0");
            return 0;
        }
    }

    const char& string::back() const {
        if (len == 0) {
            throw std::out_of_range("string::back() called on empty string");
        }
        return data[len - 1];
    }

    // 删除字符串最后一个字符
    void string::pop_back() {
        if (len == 0) {
            throw std::out_of_range("string::pop_back() called on empty string");
        }
        --len;
        data[len] = '\0';  // 可选：保证字符串以 '\0' 结尾
    }

    void string::reserve(size_t new_cap) {
        LOGV("nonstd::string.reserve(new_cap=%zu) is called", new_cap);

        if (new_cap <= capacity) {
            LOGV("nonstd::string.reserve: no need to grow (current=%zu)", capacity);
            return;
        }

        /* 新缓冲区 */
        char* new_data = new char[new_cap + 1];   // +1 for '\0'
        LOGV("nonstd::string.reserve: allocating %zu bytes", new_cap + 1);

        /* 拷贝旧内容 */
        if (data) {
            for (size_t i = 0; i < len; ++i) new_data[i] = data[i];
            delete[] data;
            LOGV("nonstd::string.reserve: copied %zu chars, freed old buffer", len);
        }

        new_data[len] = '\0';
        data     = new_data;
        capacity = new_cap;
    }

    string& string::append(const char* s, size_t n) {
        LOGV("nonstd::string.append(ptr=%p, n=%zu) is called", (void*)s, n);

        if (!s || n == 0) return *this;

        size_t needed = len + n;
        if (needed > capacity) {
            /* 至少按 2 倍策略扩容，避免频繁 realloc */
            size_t new_cap = (capacity == 0) ? needed : (capacity * 2 > needed ? capacity * 2 : needed);
            reserve(new_cap);
        }

        /* 逐字符拷贝 */
        for (size_t i = 0; i < n; ++i) data[len + i] = s[i];

        len += n;
        data[len] = '\0';

        LOGV("nonstd::string.append: appended %zu chars, new length=%zu", n, len);
        return *this;
    }

    string& string::append(const char* s) {
        if (!s) return *this;
        size_t n = 0;
        while (s[n] != '\0') ++n;   // 手动求长度
        return append(s, n);
    }

} // namespace nonstd

