#ifndef zString_H
#define zString_H
#define ZCONFIG_ENABLE_NONSTD_API 1

#include <initializer_list>
#include <algorithm>
#include <cassert>

#include "zLibc.h"

namespace nonstd {
#if ZCONFIG_ENABLE_NONSTD_API
    // 使用typedef避免宏替换
    template<typename T>
    using initializer_list = std::initializer_list<T>;

    // char_traits template class
    template<typename CharT>
    struct char_traits {
        typedef CharT     char_type;
        typedef int       int_type;
        typedef std::streamoff off_type;
        typedef std::streampos pos_type;
        typedef mbstate_t state_type;

        static inline constexpr
        void assign(char_type& c1, const char_type& c2) noexcept { c1 = c2; }
        
        static inline constexpr bool eq(char_type c1, char_type c2) noexcept {
            return c1 == c2;
        }
        
        static inline constexpr bool lt(char_type c1, char_type c2) noexcept {
            return c1 < c2;
        }

        static constexpr
        int compare(const char_type* s1, const char_type* s2, size_t n) noexcept {
            for (size_t i = 0; i < n; ++i) {
                if (lt(s1[i], s2[i])) return -1;
                if (lt(s2[i], s1[i])) return 1;
            }
            return 0;
        }
        
        static inline size_t constexpr
        length(const char_type* s) noexcept {
            size_t len = 0;
            while (s[len] != char_type(0)) ++len; // Use char_type(0) instead of char_type()
            return len;
        }
        
        static constexpr
        const char_type* find(const char_type* s, size_t n, const char_type& a) noexcept {
            for (size_t i = 0; i < n; ++i) {
                if (eq(s[i], a)) return s + i;
            }
            return nullptr;
        }
        
        static inline constexpr
        char_type* move(char_type* s1, const char_type* s2, size_t n) noexcept {
            if (n == 0) return s1;
            // Simple implementation for non-overlapping or overlapping cases
            if (s1 <= s2 || s1 >= s2 + n) {
                for (size_t i = 0; i < n; ++i) {
                    s1[i] = s2[i];
                }
            } else {
                for (size_t i = n; i > 0; --i) {
                    s1[i-1] = s2[i-1];
                }
            }
            return s1;
        }
        
        static inline constexpr
        char_type* copy(char_type* s1, const char_type* s2, size_t n) noexcept {
            for (size_t i = 0; i < n; ++i) {
                s1[i] = s2[i];
            }
            return s1;
        }
        
        static inline constexpr
        char_type* assign(char_type* s, size_t n, char_type a) noexcept {
            for (size_t i = 0; i < n; ++i) {
                s[i] = a;
            }
            return s;
        }

        static inline constexpr int_type not_eof(int_type c) noexcept {
            return eq_int_type(c, eof()) ? 0 : c;
        }
        
        static inline constexpr char_type to_char_type(int_type c) noexcept {
            return static_cast<char_type>(c);
        }
        
        static inline constexpr int_type to_int_type(char_type c) noexcept {
            return static_cast<int_type>(c);
        }
        
        static inline constexpr bool eq_int_type(int_type c1, int_type c2) noexcept {
            return c1 == c2;
        }
        
        static inline constexpr int_type eof() noexcept {
            return static_cast<int_type>(-1);
        }
    };

    // Specialization for char type with optimized implementations
    template<>
    struct char_traits<char> {
        typedef char      char_type;
        typedef int       int_type;
        typedef std::streamoff off_type;
        typedef std::streampos pos_type;
        typedef mbstate_t state_type;

        static inline constexpr
        void assign(char_type& c1, const char_type& c2) noexcept { c1 = c2; }
        
        static inline constexpr bool eq(char_type c1, char_type c2) noexcept {
            return c1 == c2;
        }
        
        static inline constexpr bool lt(char_type c1, char_type c2) noexcept {
            return static_cast<unsigned char>(c1) < static_cast<unsigned char>(c2);
        }

        static constexpr
        int compare(const char_type* s1, const char_type* s2, size_t n) noexcept {
            if (n == 0) return 0;
            return memcmp(s1, s2, n);
        }
        
        static inline size_t constexpr
        length(const char_type* s) noexcept {
            size_t len = 0;
            while (s[len] != char_type(0)) ++len; // Use char_type(0) instead of char_type()
            return len;
        }
        
        static constexpr
        const char_type* find(const char_type* s, size_t n, const char_type& a) noexcept {
            for (size_t i = 0; i < n; ++i) {
                if (s[i] == a) return s + i;
            }
            return nullptr;
        }
        
        static inline constexpr
        char_type* move(char_type* s1, const char_type* s2, size_t n) noexcept {
            if (n == 0) return s1;
            return static_cast<char_type*>(memmove(s1, s2, n));
        }
        
        static inline constexpr
        char_type* copy(char_type* s1, const char_type* s2, size_t n) noexcept {
            assert(s2 < s1 || s2 >= s1 + n);
            if (n == 0) return s1;
            return static_cast<char_type*>(memcpy(s1, s2, n));
        }
        
        static inline constexpr
        char_type* assign(char_type* s, size_t n, char_type a) noexcept {
            if (n == 0) return s;
            return static_cast<char_type*>(memset(s, to_int_type(a), n));
        }

        static inline constexpr int_type not_eof(int_type c) noexcept {
            return eq_int_type(c, eof()) ? ~eof() : c;
        }
        
        static inline constexpr char_type to_char_type(int_type c) noexcept {
            return static_cast<char_type>(c);
        }
        
        static inline constexpr int_type to_int_type(char_type c) noexcept {
            return static_cast<int_type>(static_cast<unsigned char>(c));
        }
        
        static inline constexpr bool eq_int_type(int_type c1, int_type c2) noexcept {
            return c1 == c2;
        }
        
        static inline constexpr int_type eof() noexcept {
            return static_cast<int_type>(EOF);
        }
    };

    template<typename CharT, typename Traits = char_traits<CharT>, typename Allocator = std::allocator<CharT>>
    class basic_string {
    private:
        CharT* data_;
        size_t size_;
        size_t capacity_;
        Allocator alloc_;

        void resize_capacity(size_t new_capacity) {
            CharT* new_data = alloc_.allocate(new_capacity + 1); // +1 for null terminator
            
            if (data_) {
                for (size_t i = 0; i < size_; ++i) {
                    alloc_.construct(&new_data[i], std::move(data_[i]));
                    alloc_.destroy(&data_[i]);
                }
                // Also destroy the null terminator
                alloc_.destroy(&data_[size_]);
                alloc_.deallocate(data_, capacity_ + 1); // +1 for null terminator
            }
            
            data_ = new_data;
            capacity_ = new_capacity;
            // Always ensure null terminator is present, even for empty strings
            alloc_.construct(&data_[size_], CharT(0));
        }

    public:
        // Type definitions
        typedef CharT                                        value_type;
        typedef Allocator                                    allocator_type;
        typedef typename allocator_type::size_type           size_type;
        typedef typename allocator_type::difference_type     difference_type;
        typedef typename allocator_type::reference           reference;
        typedef typename allocator_type::const_reference     const_reference;
        typedef typename allocator_type::pointer             pointer;
        typedef typename allocator_type::const_pointer       const_pointer;

        static const size_type npos = -1;

        // Iterator class
        class iterator {
        private:
            CharT* current;

        public:
            typedef std::random_access_iterator_tag iterator_category;
            typedef CharT value_type;
            typedef std::ptrdiff_t difference_type;
            typedef CharT* pointer;
            typedef CharT& reference;

            iterator(CharT* ptr) : current(ptr) {}

            CharT& operator*() {
                return *current;
            }

            iterator& operator++() {
                ++current;
                return *this;
            }

            iterator operator++(int) {
                iterator temp = *this;
                ++current;
                return temp;
            }

            iterator& operator--() {
                --current;
                return *this;
            }

            iterator operator--(int) {
                iterator temp = *this;
                --current;
                return temp;
            }

            iterator operator+(size_t n) const {
                return iterator(current + n);
            }

            iterator operator-(size_t n) const {
                return iterator(current - n);
            }

            bool operator==(const iterator& other) const {
                return current == other.current;
            }

            bool operator!=(const iterator& other) const {
                return current != other.current;
            }

            bool operator<(const iterator& other) const {
                return current < other.current;
            }

            bool operator>(const iterator& other) const {
                return current > other.current;
            }

            bool operator<=(const iterator& other) const {
                return current <= other.current;
            }

            bool operator>=(const iterator& other) const {
                return current >= other.current;
            }

            CharT* operator->() {
                return current;
            }

            const CharT* operator->() const {
                return current;
            }

            friend class basic_string;
        };

        // Const Iterator class
        class const_iterator {
        private:
            const CharT* current;

        public:
            typedef std::random_access_iterator_tag iterator_category;
            typedef CharT value_type;
            typedef std::ptrdiff_t difference_type;
            typedef const CharT* pointer;
            typedef const CharT& reference;

            const_iterator(const CharT* ptr) : current(ptr) {}
            const_iterator(const iterator& other) : current(other.current) {}

            const CharT& operator*() const {
                return *current;
            }

            const_iterator& operator++() {
                ++current;
                return *this;
            }

            const_iterator operator++(int) {
                const_iterator temp = *this;
                ++current;
                return temp;
            }

            const_iterator& operator--() {
                --current;
                return *this;
            }

            const_iterator operator--(int) {
                const_iterator temp = *this;
                --current;
                return temp;
            }

            const_iterator operator+(size_t n) const {
                return const_iterator(current + n);
            }

            const_iterator operator-(size_t n) const {
                return const_iterator(current - n);
            }

            bool operator==(const const_iterator& other) const {
                return current == other.current;
            }

            bool operator!=(const const_iterator& other) const {
                return current != other.current;
            }

            bool operator<(const const_iterator& other) const {
                return current < other.current;
            }

            bool operator>(const const_iterator& other) const {
                return current > other.current;
            }

            bool operator<=(const const_iterator& other) const {
                return current <= other.current;
            }

            bool operator>=(const const_iterator& other) const {
                return current >= other.current;
            }

            const CharT* operator->() const {
                return current;
            }

            friend class basic_string;
        };

        // Reverse iterator types
        typedef std::reverse_iterator<iterator> reverse_iterator;
        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

        // Constructors
        basic_string() noexcept : data_(nullptr), size_(0), capacity_(0), alloc_() {
            // Initialize with null terminator for empty string
            data_ = alloc_.allocate(1);
            alloc_.construct(&data_[0], CharT(0));
        }

        explicit basic_string(const allocator_type& alloc) : data_(nullptr), size_(0), capacity_(0), alloc_(alloc) {
            // Initialize with null terminator for empty string
            data_ = alloc_.allocate(1);
            alloc_.construct(&data_[0], CharT(0));
        }

        basic_string(const basic_string& str) : data_(nullptr), size_(str.size_), capacity_(str.capacity_), alloc_(str.alloc_) {
            if (capacity_ > 0) {
                data_ = alloc_.allocate(capacity_ + 1); // +1 for null terminator
                for (size_t i = 0; i < size_; ++i) {
                    alloc_.construct(&data_[i], str.data_[i]);
                }
                alloc_.construct(&data_[size_], CharT(0)); // Add null terminator
            } else {
                // Handle empty string case
                data_ = alloc_.allocate(1);
                alloc_.construct(&data_[0], CharT(0));
            }
        }

        basic_string(basic_string&& str) noexcept : data_(str.data_), size_(str.size_), capacity_(str.capacity_), alloc_(std::move(str.alloc_)) {
            str.data_ = nullptr;
            str.size_ = 0;
            str.capacity_ = 0;
        }

        basic_string(const basic_string& str, size_type pos, size_type n = npos, const allocator_type& a = allocator_type()) 
            : data_(nullptr), size_(0), capacity_(0), alloc_(a) {
            if (pos > str.size()) {
                throw std::out_of_range("basic_string::basic_string");
            }
            size_t actual_n = (n == npos) ? str.size() - pos : std::min(n, str.size() - pos);
            if (actual_n > 0) {
                capacity_ = actual_n;
                data_ = alloc_.allocate(capacity_ + 1); // +1 for null terminator
                for (size_t i = 0; i < actual_n; ++i) {
                    alloc_.construct(&data_[i], str.data_[pos + i]);
                }
                alloc_.construct(&data_[actual_n], CharT(0)); // Add null terminator
                size_ = actual_n;
            } else {
                // Handle empty substring case
                capacity_ = 0;
                data_ = alloc_.allocate(1);
                alloc_.construct(&data_[0], CharT(0));
            }
        }

        basic_string(const CharT* s, const allocator_type& a = allocator_type()) : data_(nullptr), size_(0), capacity_(0), alloc_(a) {
            if (s) {
                size_t len = 0;
                while (s[len] != CharT(0)) ++len; // Use CharT(0) instead of CharT()
                if (len > 0) {
                    capacity_ = len;
                    data_ = alloc_.allocate(capacity_ + 1); // +1 for null terminator
                    for (size_t i = 0; i < len; ++i) {
                        alloc_.construct(&data_[i], s[i]);
                    }
                    alloc_.construct(&data_[len], CharT(0)); // Add null terminator
                    size_ = len;
                } else {
                    // Handle empty string case - still need null terminator
                    capacity_ = 0;
                    data_ = alloc_.allocate(1);
                    alloc_.construct(&data_[0], CharT(0));
                }
            } else {
                // Handle null pointer case - still need null terminator
                capacity_ = 0;
                data_ = alloc_.allocate(1);
                alloc_.construct(&data_[0], CharT(0));
            }
        }

        basic_string(const CharT* s, size_type n, const allocator_type& a = allocator_type()) 
            : data_(nullptr), size_(0), capacity_(0), alloc_(a) {
            if (n > 0 && s) {
                capacity_ = n;
                data_ = alloc_.allocate(capacity_ + 1); // +1 for null terminator
                for (size_t i = 0; i < n; ++i) {
                    alloc_.construct(&data_[i], s[i]);
                }
                alloc_.construct(&data_[n], CharT(0)); // Add null terminator
                size_ = n;
            } else {
                // Handle empty case - still need null terminator
                capacity_ = 0;
                data_ = alloc_.allocate(1);
                alloc_.construct(&data_[0], CharT(0));
            }
        }

        basic_string(size_type n, CharT c, const allocator_type& a = allocator_type()) 
            : data_(nullptr), size_(0), capacity_(0), alloc_(a) {
            if (n > 0) {
                capacity_ = n;
                data_ = alloc_.allocate(capacity_ + 1); // +1 for null terminator
                for (size_t i = 0; i < n; ++i) {
                    alloc_.construct(&data_[i], c);
                }
                alloc_.construct(&data_[n], CharT(0)); // Add null terminator
                size_ = n;
            } else {
                // Handle empty case - still need null terminator
                capacity_ = 0;
                data_ = alloc_.allocate(1);
                alloc_.construct(&data_[0], CharT(0));
            }
        }

        template<class InputIterator>
        basic_string(InputIterator first, InputIterator last, const allocator_type& a = allocator_type()) 
            : data_(nullptr), size_(0), capacity_(0), alloc_(a) {
            size_t count = 0;
            for (InputIterator it = first; it != last; ++it) {
                ++count;
            }
            
            if (count > 0) {
                capacity_ = count;
                data_ = alloc_.allocate(capacity_ + 1); // +1 for null terminator
                size_t i = 0;
                for (InputIterator it = first; it != last; ++it, ++i) {
                    alloc_.construct(&data_[i], *it);
                }
                alloc_.construct(&data_[count], CharT(0)); // Add null terminator
                size_ = count;
            } else {
                // Handle empty range case
                capacity_ = 0;
                data_ = alloc_.allocate(1);
                alloc_.construct(&data_[0], CharT(0));
            }
        }

        basic_string(initializer_list<CharT> il, const allocator_type& a = allocator_type()) 
            : data_(nullptr), size_(0), capacity_(0), alloc_(a) {
            size_t init_size = il.size();
            if (init_size > 0) {
                capacity_ = init_size;
                data_ = alloc_.allocate(capacity_ + 1); // +1 for null terminator
                size_t i = 0;
                for (const auto& item : il) {
                    alloc_.construct(&data_[i], item);
                    ++i;
                }
                alloc_.construct(&data_[init_size], CharT(0)); // Add null terminator
                size_ = init_size;
            } else {
                // Handle empty case - still need null terminator
                capacity_ = 0;
                data_ = alloc_.allocate(1);
                alloc_.construct(&data_[0], CharT(0));
            }
        }

        // Destructor
        ~basic_string() {
            if (data_) {
                for (size_t i = 0; i < size_; ++i) {
                    alloc_.destroy(&data_[i]);
                }
                // Also destroy the null terminator
                alloc_.destroy(&data_[size_]);
                alloc_.deallocate(data_, capacity_ + 1); // +1 for null terminator
            }
        }

        // Assignment operators
        basic_string& operator=(const basic_string& str) {
            if (this != &str) {
                clear();
                size_ = str.size_;
                capacity_ = str.capacity_;
                alloc_ = str.alloc_;

                if (capacity_ > 0) {
                    data_ = alloc_.allocate(capacity_ + 1); // +1 for null terminator
                    for (size_t i = 0; i < size_; ++i) {
                        alloc_.construct(&data_[i], str.data_[i]);
                    }
                    alloc_.construct(&data_[size_], CharT(0)); // Add null terminator
                } else {
                    // Handle empty string case
                    data_ = alloc_.allocate(1);
                    alloc_.construct(&data_[0], CharT(0));
                }
            }
            return *this;
        }

        basic_string& operator=(basic_string&& str) noexcept {
            if (this != &str) {
                clear();
                data_ = str.data_;
                size_ = str.size_;
                capacity_ = str.capacity_;
                alloc_ = std::move(str.alloc_);

                str.data_ = nullptr;
                str.size_ = 0;
                str.capacity_ = 0;
            }
            return *this;
        }

        basic_string& operator=(const CharT* s) {
            clear();
            if (s) {
                size_t len = 0;
                while (s[len] != CharT(0)) ++len; // Use CharT(0) instead of CharT()
                if (len > 0) {
                    capacity_ = len;
                    data_ = alloc_.allocate(capacity_ + 1); // +1 for null terminator
                    for (size_t i = 0; i < len; ++i) {
                        alloc_.construct(&data_[i], s[i]);
                    }
                    alloc_.construct(&data_[len], CharT(0)); // Add null terminator
                    size_ = len;
                } else {
                    // Handle empty string case
                    capacity_ = 0;
                    data_ = alloc_.allocate(1);
                    alloc_.construct(&data_[0], CharT(0));
                }
            } else {
                // Handle null pointer case
                capacity_ = 0;
                data_ = alloc_.allocate(1);
                alloc_.construct(&data_[0], CharT(0));
            }
            return *this;
        }

        basic_string& operator=(CharT c) {
            clear();
            capacity_ = 1;
            data_ = alloc_.allocate(capacity_ + 1); // +1 for null terminator
            alloc_.construct(&data_[0], c);
            alloc_.construct(&data_[1], CharT(0)); // Add null terminator
            size_ = 1;
            return *this;
        }

        basic_string& operator=(initializer_list<CharT> il) {
            clear();
            size_t init_size = il.size();
            if (init_size > 0) {
                capacity_ = init_size;
                data_ = alloc_.allocate(capacity_ + 1); // +1 for null terminator
                size_t i = 0;
                for (const auto& item : il) {
                    alloc_.construct(&data_[i], item);
                    ++i;
                }
                alloc_.construct(&data_[init_size], CharT(0)); // Add null terminator
                size_ = init_size;
            } else {
                // Handle empty case
                capacity_ = 0;
                data_ = alloc_.allocate(1);
                alloc_.construct(&data_[0], CharT(0));
            }
            return *this;
        }

        // Allocator access
        allocator_type get_allocator() const noexcept {
            return alloc_;
        }

        // Iterators
        iterator begin() noexcept {
            return iterator(data_);
        }

        const_iterator begin() const noexcept {
            return const_iterator(data_);
        }

        iterator end() noexcept {
            return iterator(data_ + size_);
        }

        const_iterator end() const noexcept {
            return const_iterator(data_ + size_);
        }

        reverse_iterator rbegin() noexcept {
            return reverse_iterator(end());
        }

        const_reverse_iterator rbegin() const noexcept {
            return const_reverse_iterator(end());
        }

        reverse_iterator rend() noexcept {
            return reverse_iterator(begin());
        }

        const_reverse_iterator rend() const noexcept {
            return const_reverse_iterator(begin());
        }

        const_iterator cbegin() const noexcept {
            return begin();
        }

        const_iterator cend() const noexcept {
            return end();
        }

        const_reverse_iterator crbegin() const noexcept {
            return rbegin();
        }

        const_reverse_iterator crend() const noexcept {
            return rend();
        }

        // Capacity
        size_type size() const noexcept {
            return size_;
        }

        size_type length() const noexcept {
            return size_;
        }

        size_type max_size() const noexcept {
            return alloc_.max_size();
        }

        size_type capacity() const noexcept {
            return capacity_;
        }

        bool empty() const noexcept {
            return size_ == 0;
        }

        void reserve(size_type res_arg = 0) {
            if (res_arg > capacity_) {
                resize_capacity(res_arg);
            } else if (res_arg == 0 && capacity_ > 0 && size_ == 0) {
                // 处理空字符串的情况，确保有 null 终止符
                resize_capacity(0);
            }
        }

        void resize(size_type n) {
            resize(n, CharT(0));
        }

        void resize(size_type n, CharT c) {
            if (n > size_) {
                // 扩展字符串
                if (n > capacity_) {
                    size_t new_capacity = capacity_;
                    while (new_capacity < n) {
                        new_capacity *= 2;
                    }
                    resize_capacity(new_capacity);
                }

                // 用字符 c 填充新空间
                for (size_t i = size_; i < n; ++i) {
                    alloc_.construct(&data_[i], c);
                }
                // Update null terminator
                alloc_.destroy(&data_[size_]);
                alloc_.construct(&data_[n], CharT(0));
                size_ = n;
            } else if (n < size_) {
                // 缩小字符串
                for (size_t i = n; i < size_; ++i) {
                    alloc_.destroy(&data_[i]);
                }
                // Update null terminator
                alloc_.destroy(&data_[size_]);
                alloc_.construct(&data_[n], CharT(0));
                size_ = n;
            } else if (n == 0 && size_ == 0) {
                // 处理空字符串情况，确保有 null 终止符
                if (!data_) {
                    data_ = alloc_.allocate(1);
                    alloc_.construct(&data_[0], CharT(0));
                }
            }
            // 如果 n == size_ 且不为 0，什么都不做
        }

        void shrink_to_fit() noexcept {
            if (size_ < capacity_) {
                resize_capacity(size_);
            } else if (size_ == 0 && capacity_ > 0) {
                // 处理空字符串的情况
                resize_capacity(0);
            }
        }

        void clear() noexcept {
            if (data_) {
                for (size_t i = 0; i < size_; ++i) {
                    alloc_.destroy(&data_[i]);
                }
                // Also destroy the null terminator
                alloc_.destroy(&data_[size_]);
                alloc_.deallocate(data_, capacity_ + 1); // +1 for null terminator
            }
            // Always allocate space for null terminator, even for empty string
            data_ = alloc_.allocate(1);
            alloc_.construct(&data_[0], CharT(0));
            size_ = 0;
            capacity_ = 0;
        }

        // Element access
        reference operator[](size_type pos) {
            return data_[pos];
        }

        const_reference operator[](size_type pos) const {
            return data_[pos];
        }

        reference at(size_type n) {
            if (n >= size_) {
                throw std::out_of_range("basic_string::at");
            }
            return data_[n];
        }

        const_reference at(size_type n) const {
            if (n >= size_) {
                throw std::out_of_range("basic_string::at");
            }
            return data_[n];
        }

        reference front() {
            if (size_ == 0) {
                throw std::out_of_range("basic_string::front");
            }
            return data_[0];
        }

        const_reference front() const {
            if (size_ == 0) {
                throw std::out_of_range("basic_string::front");
            }
            return data_[0];
        }

        reference back() {
            if (size_ == 0) {
                throw std::out_of_range("basic_string::back");
            }
            return data_[size_ - 1];
        }

        const_reference back() const {
            if (size_ == 0) {
                throw std::out_of_range("basic_string::back");
            }
            return data_[size_ - 1];
        }

        const CharT* c_str() const noexcept {
            return data_;
        }

        const CharT* data() const noexcept {
            return data_;
        }

        CharT* data() noexcept {
            return data_;
        }

        // Modifiers
        basic_string& operator+=(const basic_string& str) {
            return append(str);
        }

        basic_string& operator+=(const CharT* s) {
            return append(s);
        }

        basic_string& operator+=(CharT c) {
            push_back(c);
            return *this;
        }

        basic_string& operator+=(initializer_list<CharT> il) {
            return append(il);
        }

        basic_string& append(const basic_string& str) {
            return append(str.data_, str.size_);
        }

        basic_string& append(const basic_string& str, size_type pos, size_type n = npos) {
            if (pos > str.size()) {
                throw std::out_of_range("basic_string::append");
            }
            size_t actual_n = (n == npos) ? str.size() - pos : std::min(n, str.size() - pos);
            return append(str.data_ + pos, actual_n);
        }

        basic_string& append(const CharT* s, size_type n) {
            if (n > 0 && s) {
                size_t new_size = size_ + n;
                if (new_size > capacity_) {
                    size_t new_capacity = (capacity_ == 0) ? n : capacity_;
                    while (new_capacity < new_size) {
                        new_capacity *= 2;
                    }
                    resize_capacity(new_capacity);
                }
                
                // Destroy old null terminator
                alloc_.destroy(&data_[size_]);
                
                for (size_t i = 0; i < n; ++i) {
                    alloc_.construct(&data_[size_ + i], s[i]);
                }
                
                // Add new null terminator
                alloc_.construct(&data_[new_size], CharT(0));
                size_ = new_size;
            }
            return *this;
        }

        basic_string& append(const CharT* s) {
            if (s) {
                size_t len = 0;
                while (s[len] != CharT(0)) ++len; // Use CharT(0) instead of CharT()
                return append(s, len);
            }
            return *this;
        }

        basic_string& append(size_type n, CharT c) {
            if (n > 0) {
                size_t new_size = size_ + n;
                if (new_size > capacity_) {
                    size_t new_capacity = (capacity_ == 0) ? n : capacity_;
                    while (new_capacity < new_size) {
                        new_capacity *= 2;
                    }
                    resize_capacity(new_capacity);
                }
                
                // Destroy old null terminator
                alloc_.destroy(&data_[size_]);
                
                for (size_t i = 0; i < n; ++i) {
                    alloc_.construct(&data_[size_ + i], c);
                }
                
                // Add new null terminator
                alloc_.construct(&data_[new_size], CharT(0));
                size_ = new_size;
            }
            return *this;
        }

        template<class InputIterator>
        basic_string& append(InputIterator first, InputIterator last) {
            size_t count = 0;
            for (InputIterator it = first; it != last; ++it) {
                ++count;
            }
            
            if (count > 0) {
                size_t new_size = size_ + count;
                if (new_size > capacity_) {
                    size_t new_capacity = (capacity_ == 0) ? count : capacity_;
                    while (new_capacity < new_size) {
                        new_capacity *= 2;
                    }
                    resize_capacity(new_capacity);
                }
                
                // Destroy old null terminator
                alloc_.destroy(&data_[size_]);
                
                size_t i = 0;
                for (InputIterator it = first; it != last; ++it, ++i) {
                    alloc_.construct(&data_[size_ + i], *it);
                }
                
                // Add new null terminator
                alloc_.construct(&data_[new_size], CharT(0));
                size_ = new_size;
            }
            return *this;
        }

        basic_string& append(initializer_list<CharT> il) {
            return append(il.begin(), il.end());
        }

        void push_back(CharT c) {
            size_t new_size = size_ + 1;
            if (new_size > capacity_) {
                size_t new_capacity = (capacity_ == 0) ? 1 : capacity_ * 2;
                resize_capacity(new_capacity);
            }
            
            // Destroy old null terminator
            alloc_.destroy(&data_[size_]);
            
            alloc_.construct(&data_[size_], c);
            alloc_.construct(&data_[new_size], CharT(0)); // Add new null terminator
            ++size_;
        }

        void pop_back() {
            if (size_ > 0) {
                alloc_.destroy(&data_[size_ - 1]);
                
                // Update null terminator
                alloc_.destroy(&data_[size_]);
                alloc_.construct(&data_[size_ - 1], CharT(0));
                --size_;
            }
            // 如果 size_ == 0，什么都不做（空字符串已经是正确的）
        }

        // Find methods
        size_type find(const basic_string& str, size_type pos = 0) const noexcept {
            return find(str.data_, pos, str.size_);
        }

        size_type find(const CharT* s, size_type pos, size_type n) const noexcept {
            if (pos >= size_ || n == 0) return npos;
            if (n > size_) return npos;  // 防止无符号数下溢
            
            for (size_t i = pos; i + n <= size_; ++i) {
                bool found = true;
                for (size_t j = 0; j < n; ++j) {
                    if (data_[i + j] != s[j]) {
                        found = false;
                        break;
                    }
                }
                if (found) return i;
            }
            return npos;
        }

        size_type find(const CharT* s, size_type pos = 0) const noexcept {
            if (!s) return npos;
            size_t len = Traits::length(s);
            return find(s, pos, len);
        }

        size_type find(CharT c, size_type pos = 0) const noexcept {
            if (pos >= size_) return npos;
            for (size_t i = pos; i < size_; ++i) {
                if (data_[i] == c) return i;
            }
            return npos;
        }

        size_type rfind(const basic_string& str, size_type pos = npos) const noexcept {
            return rfind(str.data_, pos, str.size_);
        }

        size_type rfind(const CharT* s, size_type pos, size_type n) const noexcept {
            if (n == 0) return pos;
            if (size_ == 0) return npos;  // 空字符串无法匹配
            if (n > size_) return npos;    // 防止无符号数下溢
            if (pos >= size_) pos = size_ - 1;
            if (pos < n - 1) return npos;  // pos 位置无法容纳 n 个字符
            
            for (size_t i = pos; i >= n - 1; --i) {
                bool found = true;
                for (size_t j = 0; j < n; ++j) {
                    if (data_[i - n + 1 + j] != s[j]) {
                        found = false;
                        break;
                    }
                }
                if (found) return i - n + 1;
                if (i == 0) break;  // 防止下溢（虽然理论上不会到达这里）
            }
            return npos;
        }

        size_type rfind(const CharT* s, size_type pos = npos) const noexcept {
            if (!s) return npos;
            size_t len = 0;
            while (s[len] != CharT(0)) ++len; // Use CharT(0) instead of CharT()
            return rfind(s, pos, len);
        }

        size_type rfind(CharT c, size_type pos = npos) const noexcept {
            if (pos >= size_) pos = size_ - 1;
            for (int i = static_cast<int>(pos); i >= 0; --i) {
                if (data_[i] == c) return static_cast<size_t>(i);
            }
            return npos;
        }

        size_type find_first_of(const basic_string& str, size_type pos = 0) const noexcept {
            return find_first_of(str.data_, pos, str.size_);
        }

        size_type find_first_of(const CharT* s, size_type pos, size_type n) const noexcept {
            if (pos >= size_) return npos;
            for (size_t i = pos; i < size_; ++i) {
                for (size_t j = 0; j < n; ++j) {
                    if (data_[i] == s[j]) return i;
                }
            }
            return npos;
        }

        size_type find_first_of(const CharT* s, size_type pos = 0) const noexcept {
            if (!s) return npos;
            size_t len = 0;
            while (s[len] != CharT(0)) ++len; // Use CharT(0) instead of CharT()
            return find_first_of(s, pos, len);
        }

        size_type find_first_of(CharT c, size_type pos = 0) const noexcept {
            return find(c, pos);
        }

        size_type find_last_of(const basic_string& str, size_type pos = npos) const noexcept {
            return find_last_of(str.data_, pos, str.size_);
        }

        size_type find_last_of(const CharT* s, size_type pos, size_type n) const noexcept {
            if (pos >= size_) pos = size_ - 1;
            for (int i = static_cast<int>(pos); i >= 0; --i) {
                for (size_t j = 0; j < n; ++j) {
                    if (data_[i] == s[j]) return static_cast<size_t>(i);
                }
            }
            return npos;
        }

        size_type find_last_of(const CharT* s, size_type pos = npos) const noexcept {
            if (!s) return npos;
            size_t len = 0;
            while (s[len] != CharT(0)) ++len; // Use CharT(0) instead of CharT()
            return find_last_of(s, pos, len);
        }

        size_type find_last_of(CharT c, size_type pos = npos) const noexcept {
            return rfind(c, pos);
        }

        size_type find_first_not_of(const basic_string& str, size_type pos = 0) const noexcept {
            return find_first_not_of(str.data_, pos, str.size_);
        }

        size_type find_first_not_of(const CharT* s, size_type pos, size_type n) const noexcept {
            if (pos >= size_) return npos;
            for (size_t i = pos; i < size_; ++i) {
                bool found = false;
                for (size_t j = 0; j < n; ++j) {
                    if (data_[i] == s[j]) {
                        found = true;
                        break;
                    }
                }
                if (!found) return i;
            }
            return npos;
        }

        size_type find_first_not_of(const CharT* s, size_type pos = 0) const noexcept {
            if (!s) return npos;
            size_t len = 0;
            while (s[len] != CharT(0)) ++len; // Use CharT(0) instead of CharT()
            return find_first_not_of(s, pos, len);
        }

        size_type find_first_not_of(CharT c, size_type pos = 0) const noexcept {
            if (pos >= size_) return npos;
            for (size_t i = pos; i < size_; ++i) {
                if (data_[i] != c) return i;
            }
            return npos;
        }

        size_type find_last_not_of(const basic_string& str, size_type pos = npos) const noexcept {
            return find_last_not_of(str.data_, pos, str.size_);
        }

        size_type find_last_not_of(const CharT* s, size_type pos, size_type n) const noexcept {
            if (pos >= size_) pos = size_ - 1;
            for (int i = static_cast<int>(pos); i >= 0; --i) {
                bool found = false;
                for (size_t j = 0; j < n; ++j) {
                    if (data_[i] == s[j]) {
                        found = true;
                        break;
                    }
                }
                if (!found) return static_cast<size_t>(i);
            }
            return npos;
        }

        size_type find_last_not_of(const CharT* s, size_type pos = npos) const noexcept {
            if (!s) return npos;
            size_t len = 0;
            while (s[len] != CharT(0)) ++len; // Use CharT(0) instead of CharT()
            return find_last_not_of(s, pos, len);
        }

        size_type find_last_not_of(CharT c, size_type pos = npos) const noexcept {
            if (pos >= size_) pos = size_ - 1;
            for (int i = static_cast<int>(pos); i >= 0; --i) {
                if (data_[i] != c) return static_cast<size_t>(i);
            }
            return npos;
        }

        // Compare methods
        int compare(const basic_string& str) const noexcept {
            size_t min_len = std::min(size_, str.size_);
            for (size_t i = 0; i < min_len; ++i) {
                if (data_[i] < str.data_[i]) return -1;
                if (data_[i] > str.data_[i]) return 1;
            }
            if (size_ < str.size_) return -1;
            if (size_ > str.size_) return 1;
            return 0;
        }

        int compare(size_type pos1, size_type n1, const basic_string& str) const {
            if (pos1 > size_) throw std::out_of_range("basic_string::compare");
            size_t actual_n1 = std::min(n1, size_ - pos1);
            basic_string temp(data_ + pos1, actual_n1);
            return temp.compare(str);
        }

        int compare(size_type pos1, size_type n1, const basic_string& str, size_type pos2, size_type n2 = npos) const {
            if (pos1 > size_ || pos2 > str.size_) throw std::out_of_range("basic_string::compare");
            size_t actual_n1 = std::min(n1, size_ - pos1);
            size_t actual_n2 = (n2 == npos) ? str.size_ - pos2 : std::min(n2, str.size_ - pos2);
            basic_string temp1(data_ + pos1, actual_n1);
            basic_string temp2(str.data_ + pos2, actual_n2);
            return temp1.compare(temp2);
        }

        int compare(const CharT* s) const noexcept {
            if (!s) return 1;
            size_t len = 0;
            while (s[len] != CharT(0)) ++len; // Use CharT(0) instead of CharT()
            size_t min_len = std::min(size_, len);
            for (size_t i = 0; i < min_len; ++i) {
                if (data_[i] < s[i]) return -1;
                if (data_[i] > s[i]) return 1;
            }
            if (size_ < len) return -1;
            if (size_ > len) return 1;
            return 0;
        }

        int compare(size_type pos1, size_type n1, const CharT* s) const {
            if (pos1 > size_) throw std::out_of_range("basic_string::compare");
            size_t actual_n1 = std::min(n1, size_ - pos1);
            basic_string temp(data_ + pos1, actual_n1);
            return temp.compare(s);
        }

        int compare(size_type pos1, size_type n1, const CharT* s, size_type n2) const {
            if (pos1 > size_) throw std::out_of_range("basic_string::compare");
            size_t actual_n1 = std::min(n1, size_ - pos1);
            basic_string temp1(data_ + pos1, actual_n1);
            basic_string temp2(s, n2);
            return temp1.compare(temp2);
        }

        // Substring
        basic_string substr(size_type pos = 0, size_type n = npos) const {
            if (pos > size_) throw std::out_of_range("basic_string::substr");
            size_t actual_n = (n == npos) ? size_ - pos : std::min(n, size_ - pos);
            return basic_string(data_ + pos, actual_n);
        }

        // Copy
        size_type copy(CharT* s, size_type n, size_type pos = 0) const {
            if (pos > size_) throw std::out_of_range("basic_string::copy");
            size_t actual_n = std::min(n, size_ - pos);
            for (size_t i = 0; i < actual_n; ++i) {
                s[i] = data_[pos + i];
            }
            return actual_n;
        }

        // Replace methods
        basic_string& replace(size_type pos, size_type len, const basic_string& str) {
            if (pos > size_) {
                throw std::out_of_range("basic_string::replace");
            }
            size_t actual_len = std::min(len, size_ - pos);
            size_t new_size = size_ - actual_len + str.size_;
            
            if (new_size > capacity_) {
                size_t new_capacity = capacity_;
                while (new_capacity < new_size) {
                    new_capacity *= 2;
                }
                resize_capacity(new_capacity);
            }
            
            // Destroy old null terminator
            alloc_.destroy(&data_[size_]);
            
            // Move existing elements after the replacement
            for (size_t i = pos + actual_len; i < size_; ++i) {
                alloc_.destroy(&data_[i]);
            }
            
            // Copy new string
            for (size_t i = 0; i < str.size_; ++i) {
                alloc_.construct(&data_[pos + i], str.data_[i]);
            }
            
            // Copy remaining elements
            for (size_t i = pos + actual_len; i < size_; ++i) {
                alloc_.construct(&data_[pos + str.size_ + (i - pos - actual_len)], data_[i]);
            }
            
            // Add new null terminator
            alloc_.construct(&data_[new_size], CharT(0));
            size_ = new_size;
            return *this;
        }

        basic_string& replace(size_type pos, size_type len, const CharT* s) {
            if (pos > size_) {
                throw std::out_of_range("basic_string::replace");
            }
            if (!s) return *this;
            
            size_t s_len = 0;
            while (s[s_len] != CharT(0)) ++s_len; // Use CharT(0) instead of CharT()
            
            size_t actual_len = std::min(len, size_ - pos);
            size_t new_size = size_ - actual_len + s_len;
            
            if (new_size > capacity_) {
                size_t new_capacity = capacity_;
                while (new_capacity < new_size) {
                    new_capacity *= 2;
                }
                resize_capacity(new_capacity);
            }
            
            // Destroy old null terminator
            alloc_.destroy(&data_[size_]);
            
            // Move existing elements after the replacement
            for (size_t i = pos + actual_len; i < size_; ++i) {
                alloc_.destroy(&data_[i]);
            }
            
            // Copy new string
            for (size_t i = 0; i < s_len; ++i) {
                alloc_.construct(&data_[pos + i], s[i]);
            }
            
            // Copy remaining elements
            for (size_t i = pos + actual_len; i < size_; ++i) {
                alloc_.construct(&data_[pos + s_len + (i - pos - actual_len)], data_[i]);
            }
            
            // Add new null terminator
            alloc_.construct(&data_[new_size], CharT(0));
            size_ = new_size;
            return *this;
        }

        basic_string& replace(size_type pos, size_type len, size_type count, CharT c) {
            if (pos > size_) {
                throw std::out_of_range("basic_string::replace");
            }
            
            size_t actual_len = std::min(len, size_ - pos);
            size_t new_size = size_ - actual_len + count;
            
            if (new_size > capacity_) {
                size_t new_capacity = capacity_;
                while (new_capacity < new_size) {
                    new_capacity *= 2;
                }
                resize_capacity(new_capacity);
            }
            
            // Destroy old null terminator
            alloc_.destroy(&data_[size_]);
            
            // Move existing elements after the replacement
            for (size_t i = pos + actual_len; i < size_; ++i) {
                alloc_.destroy(&data_[i]);
            }
            
            // Copy new characters
            for (size_t i = 0; i < count; ++i) {
                alloc_.construct(&data_[pos + i], c);
            }
            
            // Copy remaining elements
            for (size_t i = pos + actual_len; i < size_; ++i) {
                alloc_.construct(&data_[pos + count + (i - pos - actual_len)], data_[i]);
            }
            
            // Add new null terminator
            alloc_.construct(&data_[new_size], CharT(0));
            size_ = new_size;
            return *this;
        }

        // Swap
        void swap(basic_string& str) noexcept {
            std::swap(data_, str.data_);
            std::swap(size_, str.size_);
            std::swap(capacity_, str.capacity_);
            std::swap(alloc_, str.alloc_);
        }
    };

    // Type aliases
    typedef basic_string<char> string;
    typedef basic_string<wchar_t> wstring;

    // Comparison operators
    template <class CharT, class Traits, class Allocator>
    bool operator==(const basic_string<CharT, Traits, Allocator>& lhs,
                    const basic_string<CharT, Traits, Allocator>& rhs) noexcept {
        return lhs.compare(rhs) == 0;
    }

    template <class CharT, class Traits, class Allocator>
    bool operator==(const CharT* lhs, const basic_string<CharT, Traits, Allocator>& rhs) noexcept {
        return rhs.compare(lhs) == 0;
    }

    template <class CharT, class Traits, class Allocator>
    bool operator==(const basic_string<CharT, Traits, Allocator>& lhs, const CharT* rhs) noexcept {
        return lhs.compare(rhs) == 0;
    }

    template <class CharT, class Traits, class Allocator>
    bool operator!=(const basic_string<CharT, Traits, Allocator>& lhs,
                    const basic_string<CharT, Traits, Allocator>& rhs) noexcept {
        return !(lhs == rhs);
    }

    template <class CharT, class Traits, class Allocator>
    bool operator!=(const CharT* lhs, const basic_string<CharT, Traits, Allocator>& rhs) noexcept {
        return !(lhs == rhs);
    }

    template <class CharT, class Traits, class Allocator>
    bool operator!=(const basic_string<CharT, Traits, Allocator>& lhs, const CharT* rhs) noexcept {
        return !(lhs == rhs);
    }

    template <class CharT, class Traits, class Allocator>
    bool operator<(const basic_string<CharT, Traits, Allocator>& lhs,
                   const basic_string<CharT, Traits, Allocator>& rhs) noexcept {
        return lhs.compare(rhs) < 0;
    }

    template <class CharT, class Traits, class Allocator>
    bool operator<(const CharT* lhs, const basic_string<CharT, Traits, Allocator>& rhs) noexcept {
        return rhs.compare(lhs) > 0;
    }

    template <class CharT, class Traits, class Allocator>
    bool operator<(const basic_string<CharT, Traits, Allocator>& lhs, const CharT* rhs) noexcept {
        return lhs.compare(rhs) < 0;
    }

    template <class CharT, class Traits, class Allocator>
    bool operator>(const basic_string<CharT, Traits, Allocator>& lhs,
                   const basic_string<CharT, Traits, Allocator>& rhs) noexcept {
        return rhs < lhs;
    }

    template <class CharT, class Traits, class Allocator>
    bool operator>(const CharT* lhs, const basic_string<CharT, Traits, Allocator>& rhs) noexcept {
        return rhs < lhs;
    }

    template <class CharT, class Traits, class Allocator>
    bool operator>(const basic_string<CharT, Traits, Allocator>& lhs, const CharT* rhs) noexcept {
        return rhs < lhs;
    }

    template <class CharT, class Traits, class Allocator>
    bool operator<=(const basic_string<CharT, Traits, Allocator>& lhs,
                    const basic_string<CharT, Traits, Allocator>& rhs) noexcept {
        return !(rhs < lhs);
    }

    template <class CharT, class Traits, class Allocator>
    bool operator<=(const CharT* lhs, const basic_string<CharT, Traits, Allocator>& rhs) noexcept {
        return !(rhs < lhs);
    }

    template <class CharT, class Traits, class Allocator>
    bool operator<=(const basic_string<CharT, Traits, Allocator>& lhs, const CharT* rhs) noexcept {
        return !(rhs < lhs);
    }

    template <class CharT, class Traits, class Allocator>
    bool operator>=(const basic_string<CharT, Traits, Allocator>& lhs,
                    const basic_string<CharT, Traits, Allocator>& rhs) noexcept {
        return !(lhs < rhs);
    }

    template <class CharT, class Traits, class Allocator>
    bool operator>=(const CharT* lhs, const basic_string<CharT, Traits, Allocator>& rhs) noexcept {
        return !(lhs < rhs);
    }

    template <class CharT, class Traits, class Allocator>
    bool operator>=(const basic_string<CharT, Traits, Allocator>& lhs, const CharT* rhs) noexcept {
        return !(lhs < rhs);
    }

    // Concatenation operators
    template <class CharT, class Traits, class Allocator>
    basic_string<CharT, Traits, Allocator>
    operator+(const basic_string<CharT, Traits, Allocator>& lhs,
              const basic_string<CharT, Traits, Allocator>& rhs) {
        basic_string<CharT, Traits, Allocator> result(lhs);
        result.append(rhs);
        return result;
    }

    template <class CharT, class Traits, class Allocator>
    basic_string<CharT, Traits, Allocator>
    operator+(const CharT* lhs, const basic_string<CharT, Traits, Allocator>& rhs) {
        basic_string<CharT, Traits, Allocator> result(lhs);
        result.append(rhs);
        return result;
    }

    template <class CharT, class Traits, class Allocator>
    basic_string<CharT, Traits, Allocator>
    operator+(CharT lhs, const basic_string<CharT, Traits, Allocator>& rhs) {
        basic_string<CharT, Traits, Allocator> result(1, lhs);
        result.append(rhs);
        return result;
    }

    template <class CharT, class Traits, class Allocator>
    basic_string<CharT, Traits, Allocator>
    operator+(const basic_string<CharT, Traits, Allocator>& lhs, const CharT* rhs) {
        basic_string<CharT, Traits, Allocator> result(lhs);
        result.append(rhs);
        return result;
    }

    template <class CharT, class Traits, class Allocator>
    basic_string<CharT, Traits, Allocator>
    operator+(const basic_string<CharT, Traits, Allocator>& lhs, CharT rhs) {
        basic_string<CharT, Traits, Allocator> result(lhs);
        result.push_back(rhs);
        return result;
    }

    // Swap specialization
    template <class CharT, class Traits, class Allocator>
    void swap(basic_string<CharT, Traits, Allocator>& lhs,
              basic_string<CharT, Traits, Allocator>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
        lhs.swap(rhs);
    }

    // to_string functions
    inline string to_string(int val) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%d", val);
        return string(buffer);
    }

    inline string to_string(unsigned val) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%u", val);
        return string(buffer);
    }

    inline string to_string(long val) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%ld", val);
        return string(buffer);
    }

    inline string to_string(unsigned long val) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%lu", val);
        return string(buffer);
    }

    inline string to_string(long long val) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%lld", val);
        return string(buffer);
    }

    inline string to_string(unsigned long long val) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%llu", val);
        return string(buffer);
    }

    inline string to_string(float val) {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%f", val);
        return string(buffer);
    }

    inline string to_string(double val) {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%f", val);
        return string(buffer);
    }

    inline string to_string(long double val) {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%Lf", val);
        return string(buffer);
    }

#endif
} // namespace nonstd

#endif // zString_H
