// queue.h
#ifndef zQueue_H
#define zQueue_H

#include <initializer_list>
#include <utility>

#include "zConfig.h"  // 先包含我们的配置
#include "zLog.h"

namespace nonstd {
    // 使用typedef避免宏替换
    template<typename T>
    using initializer_list = std::initializer_list<T>;

    template<typename T>
    class queue {
    private:
        struct Node {
            T data;
            Node* next;

            Node(const T& value) : data(value), next(nullptr) {}
        };

        Node* front_;
        Node* back_;
        size_t size_;

        void clear() {
            LOGV("[queue] nonstd::queue: clear, old size=%zu", size_);
            while (front_) {
                Node* temp = front_;
                front_ = front_->next;
                delete temp;
            }
            back_ = nullptr;
            size_ = 0;
        }

    public:
        // Iterator class
        class iterator {
        private:
            Node* current;

        public:
            iterator(Node* node) : current(node) {}

            T& operator*() {
                return current->data;
            }

            const T& operator*() const {
                return current->data;
            }

            iterator& operator++() {
                if (current) {
                    current = current->next;
                }
                return *this;
            }

            iterator operator++(int) {
                iterator temp = *this;
                if (current) {
                    current = current->next;
                }
                return temp;
            }

            bool operator==(const iterator& other) const {
                return current == other.current;
            }

            bool operator!=(const iterator& other) const {
                return current != other.current;
            }

            T* operator->() {
                return &(current->data);
            }

            const T* operator->() const {
                return &(current->data);
            }

            // 友元声明，允许queue类访问私有成员
            friend class queue;
        };

        // Const Iterator class
        class const_iterator {
        private:
            const Node* current;

        public:
            const_iterator(const Node* node) : current(node) {}

            const T& operator*() const {
                return current->data;
            }

            const_iterator& operator++() {
                if (current) {
                    current = current->next;
                }
                return *this;
            }

            const_iterator operator++(int) {
                const_iterator temp = *this;
                if (current) {
                    current = current->next;
                }
                return temp;
            }

            bool operator==(const const_iterator& other) const {
                return current == other.current;
            }

            bool operator!=(const const_iterator& other) const {
                return current != other.current;
            }

            const T* operator->() const {
                return &(current->data);
            }
        };

        // Default constructor
        queue() : front_(nullptr), back_(nullptr), size_(0) {
            LOGV("[queue] nonstd::queue: default constructor");
        }

        // Constructor with initializer list
        queue(initializer_list<T> init) : front_(nullptr), back_(nullptr), size_(0) {
            LOGV("[queue] nonstd::queue: initializer_list constructor with %zu elements", init.size());

            for (const auto& item : init) {
                push(item);
            }

            LOGV("[queue] nonstd::queue: initializer_list constructor completed, size=%zu", size_);
        }

        // Copy constructor
        queue(const queue& other) : front_(nullptr), back_(nullptr), size_(0) {
            LOGV("[queue] nonstd::queue: copy constructor from queue with size=%zu", other.size_);

            // Copy all elements from other queue
            for (const auto& item : other) {
                push(item);
            }

            LOGV("[queue] nonstd::queue: copy constructor completed, new size=%zu", size_);
        }

        // Move constructor
        queue(queue&& other) noexcept : front_(other.front_), back_(other.back_), size_(other.size_) {
            LOGV("[queue] nonstd::queue: move constructor");
            other.front_ = nullptr;
            other.back_ = nullptr;
            other.size_ = 0;
        }

        // Destructor
        ~queue() {
            LOGV("[queue] nonstd::queue: destructor, size=%zu", size_);
            clear();
        }

        // Copy assignment operator
        queue& operator=(const queue& other) {
            LOGV("[queue] nonstd::queue: copy assignment from queue with size=%zu", other.size_);
            if (this != &other) {
                clear();

                // Copy all elements from other queue
                for (const auto& item : other) {
                    push(item);
                }
            }
            LOGV("[queue] nonstd::queue: copy assignment completed, new size=%zu", size_);
            return *this;
        }

        // Initializer list assignment operator
        queue& operator=(initializer_list<T> init) {
            LOGV("[queue] nonstd::queue: initializer_list assignment with %zu elements", init.size());

            clear();

            for (const auto& item : init) {
                push(item);
            }

            LOGV("[queue] nonstd::queue: initializer_list assignment completed, size=%zu", size_);
            return *this;
        }

        // Move assignment operator
        queue& operator=(queue&& other) noexcept {
            LOGV("[queue] nonstd::queue: move assignment");
            if (this != &other) {
                clear();
                front_ = other.front_;
                back_ = other.back_;
                size_ = other.size_;
                other.front_ = nullptr;
                other.back_ = nullptr;
                other.size_ = 0;
            }
            return *this;
        }

        // Element access
        T& front() {
            if (empty()) {
                static T default_value;
                return default_value;
            }
            return front_->data;
        }

        const T& front() const {
            if (empty()) {
                static T default_value;
                return default_value;
            }
            return front_->data;
        }

        T& back() {
            if (empty()) {
                static T default_value;
                return default_value;
            }
            return back_->data;
        }

        const T& back() const {
            if (empty()) {
                static T default_value;
                return default_value;
            }
            return back_->data;
        }

        // Capacity
        bool empty() const {
            return size_ == 0;
        }

        size_t size() const {
            return size_;
        }

        // Modifiers
        void push(const T& value) {
            LOGV("[queue] nonstd::queue: push, value=..., current size=%zu", size_);

            Node* newNode = new Node(value);

            if (empty()) {
                front_ = newNode;
                back_ = newNode;
            } else {
                back_->next = newNode;
                back_ = newNode;
            }

            size_++;
            LOGV("[queue] nonstd::queue: push completed, new size=%zu", size_);
        }

        // Emplace method - construct element in place
        template<typename... Args>
        void emplace(Args&&... args) {
            LOGV("[queue] nonstd::queue: emplace called");

            // Create a new element from the arguments
            T new_element(std::forward<Args>(args)...);
            push(new_element);

            LOGV("[queue] nonstd::queue: emplace completed");
        }

        void pop() {
            LOGV("[queue] nonstd::queue: pop, current size=%zu", size_);

            if (!empty()) {
                Node* temp = front_;
                front_ = front_->next;

                // If queue becomes empty after pop
                if (front_ == nullptr) {
                    back_ = nullptr;
                }

                delete temp;
                size_--;
                LOGV("[queue] nonstd::queue: pop completed, new size=%zu", size_);
            } else {
                LOGV("[queue] nonstd::queue: pop - queue is empty, no action taken");
            }
        }

        void swap(queue& other) {
            LOGV("[queue] nonstd::queue: swap called");

            Node* temp_front = front_;
            Node* temp_back = back_;
            size_t temp_size = size_;

            front_ = other.front_;
            back_ = other.back_;
            size_ = other.size_;

            other.front_ = temp_front;
            other.back_ = temp_back;
            other.size_ = temp_size;

            LOGV("[queue] nonstd::queue: swap completed");
        }

        // Iterators
        iterator begin() {
            return iterator(front_);
        }

        iterator end() {
            return iterator(nullptr);
        }

        const_iterator begin() const {
            return const_iterator(front_);
        }

        const_iterator end() const {
            return const_iterator(nullptr);
        }

        const_iterator cbegin() const {
            return const_iterator(front_);
        }

        const_iterator cend() const {
            return const_iterator(nullptr);
        }
    };

    // Non-member functions
    template<typename T>
    bool operator==(const queue<T>& lhs, const queue<T>& rhs) {
        if (lhs.size() != rhs.size()) {
            return false;
        }

        auto it1 = lhs.begin();
        auto it2 = rhs.begin();
        while (it1 != lhs.end() && it2 != rhs.end()) {
            if (*it1 != *it2) {
                return false;
            }
            ++it1;
            ++it2;
        }
        return true;
    }

    template<typename T>
    bool operator!=(const queue<T>& lhs, const queue<T>& rhs) {
        return !(lhs == rhs);
    }

    template<typename T>
    bool operator<(const queue<T>& lhs, const queue<T>& rhs) {
        auto it1 = lhs.begin();
        auto it2 = rhs.begin();
        while (it1 != lhs.end() && it2 != rhs.end()) {
            if (*it1 < *it2) {
                return true;
            }
            if (*it2 < *it1) {
                return false;
            }
            ++it1;
            ++it2;
        }
        return (it1 == lhs.end()) && (it2 != rhs.end());
    }

    template<typename T>
    bool operator<=(const queue<T>& lhs, const queue<T>& rhs) {
        return !(rhs < lhs);
    }

    template<typename T>
    bool operator>(const queue<T>& lhs, const queue<T>& rhs) {
        return rhs < lhs;
    }

    template<typename T>
    bool operator>=(const queue<T>& lhs, const queue<T>& rhs) {
        return !(lhs < rhs);
    }

    template<typename T>
    void swap(queue<T>& lhs, queue<T>& rhs) {
        lhs.swap(rhs);
    }

} // namespace nonstd

#endif // zQueue_H
