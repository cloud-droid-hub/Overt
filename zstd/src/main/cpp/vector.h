// vector.h
#ifndef zVector_H
#define zVector_H

#include <stdexcept>  // For std::out_of_range
#include <initializer_list>

#include "zConfig.h"  // 先包含我们的配置
#include "zLog.h"

namespace nonstd {
    // 使用typedef避免宏替换
    template<typename T>
    using initializer_list = std::initializer_list<T>;

    template<typename T>
    class vector {
    private:
        T* data_;
        size_t size_;
        size_t capacity_;

        void resize_capacity(size_t new_capacity) {
            LOGV("[vector] nonstd::vector: resize_capacity, old capacity=%zu, new capacity=%zu", capacity_, new_capacity);
            T* new_data = new T[new_capacity];

            if (data_) {
                for (size_t i = 0; i < size_; ++i) {
                    new_data[i] = data_[i];
                }
                delete[] data_;
            }

            data_ = new_data;
            capacity_ = new_capacity;
        }

    public:
        // Iterator class
        class iterator {
        private:
            T* current;

        public:
            iterator(T* ptr) : current(ptr) {}

            T& operator*() {
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

            T* operator->() {
                return current;
            }

            const T* operator->() const {
                return current;
            }

            // 友元声明，允许vector类访问私有成员
            friend class vector;
        };

        // Default constructor
        vector() : data_(nullptr), size_(0), capacity_(0) {
            LOGV("[vector] nonstd::vector: default constructor, size=%zu, capacity=%zu", size_, capacity_);
        }

        // Constructor with initial size and value
        vector(size_t count, const T& value) : size_(count), capacity_(count), data_(nullptr) {
            LOGV("[vector] nonstd::vector(size_t count=%zu, const T& value) is called", count);

            if (count > 0) {
                data_ = new T[count];
                for (size_t i = 0; i < count; ++i) {
                    data_[i] = value;
                }
            }

            LOGV("[vector] nonstd::vector(size_t, const T&) completed, size=%zu", size_);
        }

        // Constructor with initial size (default value)
        vector(size_t count) : size_(count), capacity_(count), data_(nullptr) {
            LOGV("[vector] nonstd::vector(size_t count=%zu) is called", count);

            if (count > 0) {
                data_ = new T[count];
                // Elements are default-constructed
            }

            LOGV("[vector] nonstd::vector(size_t) completed, size=%zu", size_);
        }

        // Constructor with C-style array
        vector(const T* arr, size_t count) : size_(count), capacity_(count), data_(nullptr) {
            LOGV("[vector] nonstd::vector(const T* arr, size_t count=%zu) is called", count);

            if (count > 0 && arr != nullptr) {
                data_ = new T[count];
                for (size_t i = 0; i < count; ++i) {
                    data_[i] = arr[i];
                }
            }

            LOGV("[vector] nonstd::vector(const T*, size_t) completed, size=%zu", size_);
        }

        // Constructor with initializer list (for brace initialization)
        vector(initializer_list<T> init) : size_(0), capacity_(0), data_(nullptr) {
            LOGV("[vector] nonstd::vector(initializer_list) is called with %zu elements", init.size());

            size_t init_size = init.size();
            if (init_size > 0) {
                capacity_ = init_size;
                data_ = new T[capacity_];

                size_t i = 0;
                for (const T& item : init) {
                    data_[i] = item;
                    i++;
                }
                size_ = init_size;
            }

            LOGV("[vector] nonstd::vector(initializer_list) completed, size=%zu", size_);
        }

        // Initializer list assignment operator
        vector& operator=(initializer_list<T> init) {
            LOGV("[vector] nonstd::vector: initializer_list assignment with %zu elements", init.size());

            clear();

            size_t init_size = init.size();
            if (init_size > 0) {
                capacity_ = init_size;
                data_ = new T[capacity_];

                size_t i = 0;
                for (const T& item : init) {
                    data_[i] = item;
                    i++;
                }
                size_ = init_size;
            }

            LOGV("[vector] nonstd::vector: initializer_list assignment completed, size=%zu", size_);
            return *this;
        }

        // Copy constructor
        vector(const vector& other) : data_(nullptr), size_(other.size_), capacity_(other.capacity_) {
            LOGV("[vector] nonstd::vector: copy constructor, size=%zu, capacity=%zu", size_, capacity_);
            if (capacity_ > 0) {
                data_ = new T[capacity_];
                for (size_t i = 0; i < size_; ++i) {
                    data_[i] = other.data_[i];
                }
            }
        }

        // Move constructor
        vector(vector&& other) noexcept : data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
            LOGV("[vector] nonstd::vector: move constructor, size=%zu, capacity=%zu", size_, capacity_);
            other.data_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
        }

        // Destructor
        ~vector() {
            LOGV("[vector] nonstd::vector: destructor, size=%zu, capacity=%zu", size_, capacity_);
            delete[] data_;
        }

        // Copy assignment operator
        vector& operator=(const vector& other) {
            LOGV("[vector] nonstd::vector: copy assignment, old size=%zu, new size=%zu", size_, other.size_);
            if (this != &other) {
                delete[] data_;
                size_ = other.size_;
                capacity_ = other.capacity_;

                if (capacity_ > 0) {
                    data_ = new T[capacity_];
                    for (size_t i = 0; i < size_; ++i) {
                        data_[i] = other.data_[i];
                    }
                } else {
                    data_ = nullptr;
                }
            }
            return *this;
        }

        // Move assignment operator
        vector& operator=(vector&& other) noexcept {
            LOGV("[vector] nonstd::vector: move assignment, old size=%zu, new size=%zu", size_, other.size_);
            if (this != &other) {
                delete[] data_;
                data_ = other.data_;
                size_ = other.size_;
                capacity_ = other.capacity_;

                other.data_ = nullptr;
                other.size_ = 0;
                other.capacity_ = 0;
            }
            return *this;
        }

        // Element access
        T& operator[](size_t index) {
            return data_[index];
        }

        const T& operator[](size_t index) const {
            return data_[index];
        }

        T& at(size_t index) {
            if (index >= size_) {
                // 简单的错误处理，不使用异常
                static T default_value;
                return default_value;
            }
            return data_[index];
        }

        const T& at(size_t index) const {
            if (index >= size_) {
                // 简单的错误处理，不使用异常
                static T default_value;
                return default_value;
            }
            return data_[index];
        }

        T& front() {
            if (empty()) {
                // 简单的错误处理，不使用异常
                static T default_value;
                return default_value;
            }
            return data_[0];
        }

        const T& front() const {
            if (empty()) {
                // 简单的错误处理，不使用异常
                static T default_value;
                return default_value;
            }
            return data_[0];
        }

        T& back() {
            if (empty()) {
                // 简单的错误处理，不使用异常
                static T default_value;
                return default_value;
            }
            return data_[size_ - 1];
        }

        const T& back() const {
            if (empty()) {
                // 简单的错误处理，不使用异常
                static T default_value;
                return default_value;
            }
            return data_[size_ - 1];
        }

        // Capacity
        bool empty() const {
            return size_ == 0;
        }

        size_t size() const {
            return size_;
        }

        size_t capacity() const {
            return capacity_;
        }

        // Data access
        T* data() {
            return data_;
        }

        const T* data() const {
            return data_;
        }

        void reserve(size_t new_capacity) {
            LOGV("[vector] nonstd::vector: reserve, old capacity=%zu, new capacity=%zu", capacity_, new_capacity);
            if (new_capacity > capacity_) {
                resize_capacity(new_capacity);
            }
        }

        // Modifiers
        void clear() {
            LOGV("[vector] nonstd::vector: clear, old size=%zu", size_);
            delete[] data_;
            data_ = nullptr;
            size_ = 0;
            capacity_ = 0;
        }

        // Add element to the end
        void push_back(const T& value) {
            LOGV("[vector] nonstd::vector.push_back() is called, current size=%zu, capacity=%zu", size_, capacity_);

            if (size_ >= capacity_) {
                resize_capacity(capacity_ == 0 ? 1 : capacity_ * 2);
            }

            data_[size_] = value;
            size_++;

            LOGV("[vector] nonstd::vector.push_back() completed, new size=%zu", size_);
        }

        // Construct element in place at the end
        template<typename... Args>
        void emplace_back(Args&&... args) {
            LOGV("[vector] nonstd::vector.emplace_back() is called, current size=%zu, capacity=%zu", size_, capacity_);

            if (size_ >= capacity_) {
                resize_capacity(capacity_ == 0 ? 1 : capacity_ * 2);
            }

            // Use perfect forwarding to construct the element in place
            data_[size_] = T(std::forward<Args>(args)...);

            size_++;

            LOGV("[vector] nonstd::vector.emplace_back() completed, new size=%zu", size_);
        }

        void pop_back() {
            LOGV("[vector] nonstd::vector: pop_back, size(before)=%zu", size_);
            if (!empty()) {
                --size_;
            }
            LOGV("[vector] nonstd::vector: pop_back done, size(after)=%zu", size_);
        }

        void resize(size_t count, const T& value = T()) {
            LOGV("[vector] nonstd::vector: resize, old size=%zu, new size=%zu", size_, count);
            if (count > capacity_) {
                LOGV("[vector] nonstd::vector: resize triggers resize_capacity to %zu", count);
                resize_capacity(count);
            }

            if (count > size_) {
                for (size_t i = size_; i < count; ++i) {
                    data_[i] = value;
                }
            }
            size_ = count;
            LOGV("[vector] nonstd::vector: resize done, size=%zu", size_);
        }


        void insert(iterator pos, const T& value) {
            size_t index = pos - begin();
            LOGV("[vector] nonstd::vector: insert at index=%zu, value=..., size(before)=%zu", index, size_);
            if (size_ >= capacity_) {
                size_t new_capacity = (capacity_ == 0) ? 1 : capacity_ * 2;
                LOGV("[vector] nonstd::vector: insert triggers resize_capacity to %zu", new_capacity);
                resize_capacity(new_capacity);
            }

            // Shift elements to make room
            for (size_t i = size_; i > index; --i) {
                data_[i] = data_[i - 1];
            }

            data_[index] = value;
            ++size_;
            LOGV("[vector] nonstd::vector: insert done, size(after)=%zu", size_);
        }

        // Insert range of elements from iterators
        template<typename InputIt>
        void insert(iterator pos, InputIt first, InputIt last) {
            size_t index = pos - begin();
            size_t count = 0;

            // Count the number of elements to insert
            for (InputIt it = first; it != last; ++it) {
                ++count;
            }

            LOGV("[vector] nonstd::vector: insert range at index=%zu, count=%zu, size(before)=%zu", index, count, size_);

            if (count == 0) {
                LOGV("[vector] nonstd::vector: insert range - no elements to insert");
                return;
            }

            // Ensure enough capacity
            if (size_ + count > capacity_) {
                size_t new_capacity = (capacity_ == 0) ? count : capacity_;
                while (new_capacity < size_ + count) {
                    new_capacity *= 2;
                }
                LOGV("[vector] nonstd::vector: insert range triggers resize_capacity to %zu", new_capacity);
                resize_capacity(new_capacity);
            }

            // Shift existing elements to make room
            for (size_t i = size_; i > index; --i) {
                data_[i + count - 1] = data_[i - 1];
            }

            // Insert the new elements
            size_t i = index;
            for (InputIt it = first; it != last; ++it, ++i) {
                data_[i] = *it;
            }

            size_ += count;
            LOGV("[vector] nonstd::vector: insert range done, size(after)=%zu", size_);
        }

        iterator erase(iterator pos) {
            size_t index = pos - begin();
            LOGV("[vector] nonstd::vector: erase at index=%zu, size(before)=%zu", index, size_);

            if (index < size_) {
                // Shift elements to fill the gap
                for (size_t i = index; i < size_ - 1; ++i) {
                    data_[i] = data_[i + 1];
                }
                --size_;
                LOGV("[vector] nonstd::vector: erase done, size(after)=%zu", size_);

                // 返回指向删除位置的迭代器
                if (index < size_) {
                    return iterator(data_ + index);  // 指向删除位置的下一个元素
                } else {
                    return end();  // 如果删除的是最后一个元素，返回end()
                }
            }

            LOGV("[vector] nonstd::vector: erase - invalid index, no change");
            return pos;  // 如果索引无效，返回原迭代器
        }

        // Iterators
        iterator begin() {
            return iterator(data_);
        }

        iterator end() {
            return iterator(data_ + size_);
        }

        iterator begin() const {
            return iterator(data_);
        }

        iterator end() const {
            return iterator(data_ + size_);
        }

        // Iterator arithmetic
        friend iterator operator+(size_t n, const iterator& it) {
            return iterator(it.current + n);
        }

        friend size_t operator-(const iterator& it1, const iterator& it2) {
            return it1.current - it2.current;
        }
    };

} // namespace nonstd

#endif // zVector_H
