// vector.h
#ifndef zVector_H
#define zVector_H

#include <stdexcept>  // For std::out_of_range
#include <initializer_list>
#include <memory>
#include <iterator>
#include <algorithm>

namespace nonstd {
    // 使用typedef避免宏替换
    template<typename T>
    using initializer_list = std::initializer_list<T>;

    template<typename T, typename Allocator = std::allocator<T>>
    class vector {
    private:
        T* data_;
        size_t size_;
        size_t capacity_;
        Allocator alloc_;

        void resize_capacity(size_t new_capacity) {
            T* new_data = alloc_.allocate(new_capacity);

            if (data_) {
                for (size_t i = 0; i < size_; ++i) {
                    alloc_.construct(&new_data[i], std::move(data_[i]));
                    alloc_.destroy(&data_[i]);
                }
                alloc_.deallocate(data_, capacity_);
            }

            data_ = new_data;
            capacity_ = new_capacity;
        }

    public:
        // Type definitions
        typedef T                                        value_type;
        typedef Allocator                                allocator_type;
        typedef typename allocator_type::reference       reference;
        typedef typename allocator_type::const_reference const_reference;
        typedef typename allocator_type::size_type       size_type;
        typedef typename allocator_type::difference_type difference_type;
        typedef typename allocator_type::pointer         pointer;
        typedef typename allocator_type::const_pointer   const_pointer;


        // Iterator class
        class iterator {
        private:
            T* current;

        public:
            typedef std::random_access_iterator_tag iterator_category;
            typedef T value_type;
            typedef std::ptrdiff_t difference_type;
            typedef T* pointer;
            typedef T& reference;

            iterator() : current(nullptr) {}
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

            operator T*() const {
                return current;
            }

            iterator& operator+=(difference_type n) {
                current += n;
                return *this;
            }

            iterator& operator-=(difference_type n) {
                current -= n;
                return *this;
            }

            // 友元声明，允许vector类访问私有成员
            friend class vector;
        };

        // Const Iterator class
        class const_iterator {
        private:
            const T* current;

        public:
            typedef std::random_access_iterator_tag iterator_category;
            typedef T value_type;
            typedef std::ptrdiff_t difference_type;
            typedef const T* pointer;
            typedef const T& reference;

            const_iterator() : current(nullptr) {}
            const_iterator(const T* ptr) : current(ptr) {}
            const_iterator(const iterator& other) : current(other.current) {}

            const T& operator*() const {
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

            const T* operator->() const {
                return current;
            }

            friend difference_type operator-(const const_iterator& lhs, const const_iterator& rhs) {
                return lhs.current - rhs.current;
            }

            friend difference_type operator-(const iterator& lhs, const const_iterator& rhs) {
                return lhs.current - rhs.current;
            }

            friend difference_type operator-(const const_iterator& lhs, const iterator& rhs) {
                return lhs.current - rhs.current;
            }

            // 友元声明，允许vector类访问私有成员
            friend class vector;
        };

        // Reverse iterator types
        typedef std::reverse_iterator<iterator> reverse_iterator;
        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

        // Constructors
        vector() noexcept : data_(nullptr), size_(0), capacity_(0), alloc_() {}

        explicit vector(const allocator_type& alloc) : data_(nullptr), size_(0), capacity_(0), alloc_(alloc) {}

        explicit vector(size_type n) : data_(nullptr), size_(n), capacity_(n), alloc_() {
            if (n > 0) {
                data_ = alloc_.allocate(n);
                for (size_t i = 0; i < n; ++i) {
                    alloc_.construct(&data_[i]);
                }
            }
        }

        explicit vector(size_type n, const allocator_type& alloc) : data_(nullptr), size_(n), capacity_(n), alloc_(alloc) {
            if (n > 0) {
                data_ = alloc_.allocate(n);
                for (size_t i = 0; i < n; ++i) {
                    alloc_.construct(&data_[i]);
                }
            }
        }

        vector(size_type n, const value_type& value, const allocator_type& alloc = allocator_type()) 
            : data_(nullptr), size_(n), capacity_(n), alloc_(alloc) {
            if (n > 0) {
                data_ = alloc_.allocate(n);
                for (size_t i = 0; i < n; ++i) {
                    alloc_.construct(&data_[i], value);
                }
            }
        }

        template <class InputIterator>
        vector(InputIterator first, InputIterator last, const allocator_type& alloc = allocator_type()) 
            : data_(nullptr), size_(0), capacity_(0), alloc_(alloc) {
            size_t count = 0;
            for (InputIterator it = first; it != last; ++it) {
                ++count;
            }
            
            if (count > 0) {
                capacity_ = count;
                data_ = alloc_.allocate(capacity_);
                for (InputIterator it = first; it != last; ++it) {
                    alloc_.construct(&data_[size_], *it);
                    ++size_;
                }
            }
        }

        vector(const vector& x) : data_(nullptr), size_(x.size_), capacity_(x.capacity_), alloc_(x.alloc_) {
            if (capacity_ > 0) {
                data_ = alloc_.allocate(capacity_);
                for (size_t i = 0; i < size_; ++i) {
                    alloc_.construct(&data_[i], x.data_[i]);
                }
            }
        }

        vector(vector&& x) noexcept : data_(x.data_), size_(x.size_), capacity_(x.capacity_), alloc_(std::move(x.alloc_)) {
            x.data_ = nullptr;
            x.size_ = 0;
            x.capacity_ = 0;
        }

        vector(initializer_list<value_type> il) : data_(nullptr), size_(0), capacity_(0), alloc_() {
            size_t init_size = il.size();
            if (init_size > 0) {
                capacity_ = init_size;
                data_ = alloc_.allocate(capacity_);
                size_t i = 0;
                for (const auto& item : il) {
                    alloc_.construct(&data_[i], item);
                    ++i;
                }
                size_ = init_size;
            }
        }

        vector(initializer_list<value_type> il, const allocator_type& a) : data_(nullptr), size_(0), capacity_(0), alloc_(a) {
            size_t init_size = il.size();
            if (init_size > 0) {
                capacity_ = init_size;
                data_ = alloc_.allocate(capacity_);
                size_t i = 0;
                for (const auto& item : il) {
                    alloc_.construct(&data_[i], item);
                    ++i;
                }
                size_ = init_size;
            }
        }

        // Destructor
        ~vector() {
            if (data_) {
                for (size_t i = 0; i < size_; ++i) {
                    alloc_.destroy(&data_[i]);
                }
                alloc_.deallocate(data_, capacity_);
            }
        }

        // Assignment operators
        vector& operator=(const vector& x) {
            if (this != &x) {
                clear();
                size_ = x.size_;
                capacity_ = x.capacity_;
                alloc_ = x.alloc_;

                if (capacity_ > 0) {
                    data_ = alloc_.allocate(capacity_);
                    for (size_t i = 0; i < size_; ++i) {
                        alloc_.construct(&data_[i], x.data_[i]);
                    }
                }
            }
            return *this;
        }

        vector& operator=(vector&& x) noexcept {
            if (this != &x) {
                clear();
                data_ = x.data_;
                size_ = x.size_;
                capacity_ = x.capacity_;
                alloc_ = std::move(x.alloc_);

                x.data_ = nullptr;
                x.size_ = 0;
                x.capacity_ = 0;
            }
            return *this;
        }

        vector& operator=(initializer_list<value_type> il) {
            clear();
            size_t init_size = il.size();
            if (init_size > 0) {
                capacity_ = init_size;
                data_ = alloc_.allocate(capacity_);
                size_t i = 0;
                for (const auto& item : il) {
                    alloc_.construct(&data_[i], item);
                    ++i;
                }
                size_ = init_size;
            }
            return *this;
        }

        // Assign methods
        template <class InputIterator>
        void assign(InputIterator first, InputIterator last) {
            clear();
            size_t count = 0;
            for (InputIterator it = first; it != last; ++it) {
                ++count;
            }
            
            if (count > 0) {
                capacity_ = count;
                data_ = alloc_.allocate(capacity_);
                for (InputIterator it = first; it != last; ++it) {
                    alloc_.construct(&data_[size_], *it);
                    ++size_;
                }
            }
        }

        void assign(size_type n, const value_type& u) {
            clear();
            if (n > 0) {
                capacity_ = n;
                data_ = alloc_.allocate(capacity_);
                for (size_t i = 0; i < n; ++i) {
                    alloc_.construct(&data_[i], u);
                }
                size_ = n;
            }
        }

        void assign(initializer_list<value_type> il) {
            clear();
            size_t init_size = il.size();
            if (init_size > 0) {
                capacity_ = init_size;
                data_ = alloc_.allocate(capacity_);
                size_t i = 0;
                for (const auto& item : il) {
                    alloc_.construct(&data_[i], item);
                    ++i;
                }
                size_ = init_size;
            }
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

        size_type max_size() const noexcept {
            return alloc_.max_size();
        }

        size_type capacity() const noexcept {
            return capacity_;
        }

        bool empty() const noexcept {
            return size_ == 0;
        }

        void reserve(size_type n) {
            if (n > capacity_) {
                resize_capacity(n);
            }
        }

        void shrink_to_fit() noexcept {
            if (size_ < capacity_) {
                resize_capacity(size_);
            }
        }

        // Element access
        reference operator[](size_type n) {
            return data_[n];
        }

        const_reference operator[](size_type n) const {
            return data_[n];
        }

        reference at(size_type n) {
            if (n >= size_) {
                throw std::out_of_range("vector::at");
            }
            return data_[n];
        }

        const_reference at(size_type n) const {
            if (n >= size_) {
                throw std::out_of_range("vector::at");
            }
            return data_[n];
        }

        reference front() {
            if (empty()) {
                throw std::out_of_range("vector::front - container is empty");
            }
            return data_[0];
        }

        const_reference front() const {
            if (empty()) {
                throw std::out_of_range("vector::front - container is empty");
            }
            return data_[0];
        }

        reference back() {
            if (empty()) {
                throw std::out_of_range("vector::back - container is empty");
            }
            return data_[size_ - 1];
        }

        const_reference back() const {
            if (empty()) {
                throw std::out_of_range("vector::back - container is empty");
            }
            return data_[size_ - 1];
        }

        value_type* data() noexcept {
            return data_;
        }

        const value_type* data() const noexcept {
            return data_;
        }

        // Modifiers
        void push_back(const value_type& x) {
            if (size_ >= capacity_) {
                resize_capacity(capacity_ == 0 ? 1 : capacity_ * 2);
            }
            alloc_.construct(&data_[size_], x);
            ++size_;
        }

        void push_back(value_type&& x) {
            if (size_ >= capacity_) {
                resize_capacity(capacity_ == 0 ? 1 : capacity_ * 2);
            }
            alloc_.construct(&data_[size_], std::move(x));
            ++size_;
        }

        template <class... Args>
        reference emplace_back(Args&&... args) {
            if (size_ >= capacity_) {
                resize_capacity(capacity_ == 0 ? 1 : capacity_ * 2);
            }
            alloc_.construct(&data_[size_], std::forward<Args>(args)...);
            ++size_;
            return data_[size_ - 1];
        }

        void pop_back() {
            if (!empty()) {
                alloc_.destroy(&data_[size_ - 1]);
                --size_;
            }
        }

        template <class... Args>
        iterator emplace(const_iterator position, Args&&... args) {
            size_t index = position - begin();
            if (size_ >= capacity_) {
                resize_capacity(capacity_ == 0 ? 1 : capacity_ * 2);
            }
            
            // Shift elements to make room
            for (size_t i = size_; i > index; --i) {
                alloc_.construct(&data_[i], std::move(data_[i - 1]));
                alloc_.destroy(&data_[i - 1]);
            }
            
            alloc_.construct(&data_[index], std::forward<Args>(args)...);
            ++size_;
            return iterator(&data_[index]);
        }

        iterator insert(const_iterator position, const value_type& x) {
            size_t index = position - begin();
            if (size_ >= capacity_) {
                resize_capacity(capacity_ == 0 ? 1 : capacity_ * 2);
            }
            
            // Shift elements to make room
            for (size_t i = size_; i > index; --i) {
                alloc_.construct(&data_[i], std::move(data_[i - 1]));
                alloc_.destroy(&data_[i - 1]);
            }
            
            alloc_.construct(&data_[index], x);
            ++size_;
            return iterator(&data_[index]);
        }

        iterator insert(const_iterator position, value_type&& x) {
            size_t index = position - begin();
            if (size_ >= capacity_) {
                resize_capacity(capacity_ == 0 ? 1 : capacity_ * 2);
            }
            
            // Shift elements to make room
            for (size_t i = size_; i > index; --i) {
                alloc_.construct(&data_[i], std::move(data_[i - 1]));
                alloc_.destroy(&data_[i - 1]);
            }
            
            alloc_.construct(&data_[index], std::move(x));
            ++size_;
            return iterator(&data_[index]);
        }

        iterator insert(const_iterator position, size_type n, const value_type& x) {
            size_t index = position - begin();
            if (size_ + n > capacity_) {
                size_t new_capacity = (capacity_ == 0) ? n : capacity_;
                while (new_capacity < size_ + n) {
                    new_capacity *= 2;
                }
                resize_capacity(new_capacity);
            }
            
            // Shift elements to make room
            for (size_t i = size_; i > index; --i) {
                alloc_.construct(&data_[i + n - 1], std::move(data_[i - 1]));
                alloc_.destroy(&data_[i - 1]);
            }
            
            // Insert the new elements
            for (size_t i = 0; i < n; ++i) {
                alloc_.construct(&data_[index + i], x);
            }
            
            size_ += n;
            return iterator(&data_[index]);
        }

        template <class InputIterator>
        iterator insert(const_iterator position, InputIterator first, InputIterator last) {
            size_t index = position - begin();
            size_t count = 0;
            for (InputIterator it = first; it != last; ++it) {
                ++count;
            }
            
            if (count == 0) {
                return iterator(&data_[index]);
            }
            
            if (size_ + count > capacity_) {
                size_t new_capacity = (capacity_ == 0) ? count : capacity_;
                while (new_capacity < size_ + count) {
                    new_capacity *= 2;
                }
                resize_capacity(new_capacity);
            }
            
            // Shift elements to make room
            for (size_t i = size_; i > index; --i) {
                alloc_.construct(&data_[i + count - 1], std::move(data_[i - 1]));
                alloc_.destroy(&data_[i - 1]);
            }
            
            // Insert the new elements
            size_t i = index;
            for (InputIterator it = first; it != last; ++it, ++i) {
                alloc_.construct(&data_[i], *it);
            }
            
            size_ += count;
            return iterator(&data_[index]);
        }

        iterator insert(const_iterator position, initializer_list<value_type> il) {
            return insert(position, il.begin(), il.end());
        }

        iterator erase(const_iterator position) {
            size_t index = position - begin();
            if (index < size_) {
                alloc_.destroy(&data_[index]);
                
                // Shift elements to fill the gap
                for (size_t i = index; i < size_ - 1; ++i) {
                    alloc_.construct(&data_[i], std::move(data_[i + 1]));
                    alloc_.destroy(&data_[i + 1]);
                }
                --size_;
                
                if (index < size_) {
                    return iterator(&data_[index]);
                } else {
                    return end();
                }
            }
            return iterator(&data_[index]);
        }

        iterator erase(const_iterator first, const_iterator last) {
            size_t first_index = first - begin();
            size_t last_index = last - begin();
            size_t count = last_index - first_index;
            
            if (count > 0) {
                // Destroy elements in range
                for (size_t i = first_index; i < last_index; ++i) {
                    alloc_.destroy(&data_[i]);
                }
                
                // Shift elements to fill the gap
                for (size_t i = last_index; i < size_; ++i) {
                    alloc_.construct(&data_[i - count], std::move(data_[i]));
                    alloc_.destroy(&data_[i]);
                }
                
                size_ -= count;
            }
            
            return iterator(&data_[first_index]);
        }

        void clear() noexcept {
            if (data_) {
                for (size_t i = 0; i < size_; ++i) {
                    alloc_.destroy(&data_[i]);
                }
                alloc_.deallocate(data_, capacity_);
                data_ = nullptr;
                size_ = 0;
                capacity_ = 0;
            }
        }

        void resize(size_type sz) {
            resize(sz, value_type());
        }

        void resize(size_type sz, const value_type& c) {
            if (sz > size_) {
                if (sz > capacity_) {
                    resize_capacity(sz);
                }
                for (size_t i = size_; i < sz; ++i) {
                    alloc_.construct(&data_[i], c);
                }
            } else if (sz < size_) {
                for (size_t i = sz; i < size_; ++i) {
                    alloc_.destroy(&data_[i]);
                }
            }
            size_ = sz;
        }

        void swap(vector& other) noexcept {
            std::swap(data_, other.data_);
            std::swap(size_, other.size_);
            std::swap(capacity_, other.capacity_);
            std::swap(alloc_, other.alloc_);
        }

        // Iterator arithmetic
        friend iterator operator+(size_t n, const iterator& it) {
            return iterator(it.current + n);
        }

        friend difference_type operator-(const iterator& it1, const iterator& it2) {
            return it1.current - it2.current;
        }
    };

    // Comparison operators
    template <class T, class Allocator>
    bool operator==(const vector<T, Allocator>& x, const vector<T, Allocator>& y) {
        if (x.size() != y.size()) return false;
        for (size_t i = 0; i < x.size(); ++i) {
            if (x[i] != y[i]) return false;
        }
        return true;
    }

    template <class T, class Allocator>
    bool operator<(const vector<T, Allocator>& x, const vector<T, Allocator>& y) {
        size_t min_size = std::min(x.size(), y.size());
        for (size_t i = 0; i < min_size; ++i) {
            if (x[i] < y[i]) return true;
            if (y[i] < x[i]) return false;
        }
        return x.size() < y.size();
    }

    template <class T, class Allocator>
    bool operator!=(const vector<T, Allocator>& x, const vector<T, Allocator>& y) {
        return !(x == y);
    }

    template <class T, class Allocator>
    bool operator>(const vector<T, Allocator>& x, const vector<T, Allocator>& y) {
        return y < x;
    }

    template <class T, class Allocator>
    bool operator>=(const vector<T, Allocator>& x, const vector<T, Allocator>& y) {
        return !(x < y);
    }

    template <class T, class Allocator>
    bool operator<=(const vector<T, Allocator>& x, const vector<T, Allocator>& y) {
        return !(y < x);
    }

    // Swap specialization
    template <class T, class Allocator>
    void swap(vector<T, Allocator>& x, vector<T, Allocator>& y) noexcept(noexcept(x.swap(y))) {
        x.swap(y);
    }

} // namespace nonstd

#endif // zVector_H

