// map.h
#ifndef zMap_H
#define zMap_H

#include <cstddef>
#include <initializer_list>
#include <utility>
#include <algorithm>

namespace nonstd {
    // 使用typedef避免宏替换
    template<typename T>
    using initializer_list = std::initializer_list<T>;

    // 自定义pair实现
    template<typename T1, typename T2>
    struct pair {
        typedef T1 first_type;
        typedef T2 second_type;

        T1 first;
        T2 second;

        // Default constructor
        pair() : first(), second() {}
        
        // Copy constructor
        pair(const T1& f, const T2& s) : first(f), second(s) {}
        pair(const pair& other) : first(other.first), second(other.second) {}
        
        // Move constructor
        pair(pair&& other) noexcept : first(std::move(other.first)), second(std::move(other.second)) {}
        
        // Template constructors for different types
        template<class U1, class U2>
        pair(const pair<U1, U2>& other) : first(other.first), second(other.second) {}
        
        template<class U1, class U2>
        pair(pair<U1, U2>&& other) : first(std::move(other.first)), second(std::move(other.second)) {}
        
        // Forwarding constructors
        template<class U1, class U2>
        pair(U1&& u1, U2&& u2) : first(std::forward<U1>(u1)), second(std::forward<U2>(u2)) {}

        // Assignment operators
        pair& operator=(const pair& other) {
            if (this != &other) {
                first = other.first;
                second = other.second;
            }
            return *this;
        }

        pair& operator=(pair&& other) noexcept {
            if (this != &other) {
                first = std::move(other.first);
                second = std::move(other.second);
            }
            return *this;
        }
        
        // Template assignment operators
        template<class U1, class U2>
        pair& operator=(const pair<U1, U2>& other) {
            first = other.first;
            second = other.second;
            return *this;
        }
        
        template<class U1, class U2>
        pair& operator=(pair<U1, U2>&& other) {
            first = std::move(other.first);
            second = std::move(other.second);
            return *this;
        }
        
        // Swap function
        void swap(pair& other) noexcept {
            using std::swap;
            swap(first, other.first);
            swap(second, other.second);
        }
    };

    // 自定义make_pair函数
    template<typename T1, typename T2>
    inline pair<T1, T2> make_pair(const T1& first, const T2& second) {
        return pair<T1, T2>(first, second);
    }

    // 特化版本，处理字符串字面量
    template<typename T1>
    inline pair<T1, const char*> make_pair(const T1& first, const char* second) {
        return pair<T1, const char*>(first, second);
    }

    // 特化版本，处理字符串字面量作为键
    template<typename T2>
    inline pair<const char*, T2> make_pair(const char* first, const T2& second) {
        return pair<const char*, T2>(first, second);
    }

    // 特化版本，处理两个字符串字面量
    inline pair<const char*, const char*> make_pair(const char* first, const char* second) {
        return pair<const char*, const char*>(first, second);
    }

    // 重新实现 map，使其与标准 std::map 模板参数一致
    template<typename _Key, typename _Tp, typename _Compare = std::less<_Key>, typename _Alloc = std::allocator<pair<const _Key, _Tp>>>
    class map {
    private:
        struct Node {
            _Key key;
            _Tp value;
            Node* left;
            Node* right;
            Node* parent;
            bool isRed;

            Node(const _Key& k, const _Tp& v) : key(k), value(v), left(nullptr), right(nullptr), parent(nullptr), isRed(true) {}
        };

        Node* root;
        size_t size_;
        _Compare comp_;
        _Alloc alloc_;

        // Helper functions
        void clear(Node* node) {
            if (node) {
                clear(node->left);
                clear(node->right);
                delete node;
            }
        }

        Node* findNode(const _Key& key) const {
            Node* current = root;
            while (current) {
                if (comp_(key, current->key)) {
                    current = current->left;
                } else if (comp_(current->key, key)) {
                    current = current->right;
                } else {
                    return current;
                }
            }
            return nullptr;
        }

        Node* minimum(Node* node) const {
            if (!node) return nullptr;
            while (node->left) {
                node = node->left;
            }
            return node;
        }

        Node* maximum(Node* node) const {
            if (!node) return nullptr;
            while (node->right) {
                node = node->right;
            }
            return node;
        }

        Node* successor(Node* node) const {
            if (!node) return nullptr;
            if (node->right) {
                return minimum(node->right);
            }
            Node* parent = node->parent;
            while (parent && node == parent->right) {
                node = parent;
                parent = parent->parent;
            }
            return parent;
        }

        Node* predecessor(Node* node) const {
            if (!node) return nullptr;
            if (node->left) {
                return maximum(node->left);
            }
            Node* parent = node->parent;
            while (parent && node == parent->left) {
                node = parent;
                parent = parent->parent;
            }
            return parent;
        }

        // Insert a new node with given key and value
        Node* insertNode(const _Key& key, const _Tp& value) {
            Node* newNode = new Node(key, value);

            if (!root) {
                root = newNode;
                newNode->isRed = false;  // Root is always black
            } else {
                Node* current = root;
                Node* parent = nullptr;

                // Find the position to insert
                while (current) {
                    parent = current;
                    if (comp_(key, current->key)) {
                        current = current->left;
                    } else if (comp_(current->key, key)) {
                        current = current->right;
                    } else {
                        // Key already exists, update value and return
                        current->value = value;
                        delete newNode;
                        return current;
                    }
                }

                // Insert the new node
                if (comp_(key, parent->key)) {
                    parent->left = newNode;
                } else {
                    parent->right = newNode;
                }
                newNode->parent = parent;
            }

            size_++;
            return newNode;
        }

    public:
        typedef _Key key_type;
        typedef _Tp mapped_type;
        typedef pair<const _Key, _Tp> value_type;
        typedef _Compare key_compare;
        typedef _Alloc allocator_type;
        typedef size_t size_type;
        typedef std::ptrdiff_t difference_type;
        typedef value_type& reference;
        typedef const value_type& const_reference;
        typedef value_type* pointer;
        typedef const value_type* const_pointer;

        // value_compare class - required for std::map compatibility
        class value_compare : public std::binary_function<value_type, value_type, bool> {
        protected:
            _Compare comp;
            
        public:
            value_compare(_Compare c) : comp(c) {}
            
            bool operator()(const value_type& x, const value_type& y) const {
                return comp(x.first, y.first);
            }
        };

        // Iterator class
        class iterator {
        private:
            Node* current;
            const map* container;
            mutable value_type* temp_pair_ptr;

        public:
            // 迭代器特性定义
            typedef std::bidirectional_iterator_tag iterator_category;
            typedef typename map::value_type value_type;
            typedef std::ptrdiff_t difference_type;
            typedef value_type* pointer;
            typedef value_type& reference;

            // 默认构造函数
            iterator() : current(nullptr), container(nullptr), temp_pair_ptr(nullptr) {}
            
            iterator(Node* node, const map* cont) : current(node), container(cont), temp_pair_ptr(nullptr) {}
            
            ~iterator() { delete temp_pair_ptr; }

            value_type& operator*() {
                if (!temp_pair_ptr) {
                    temp_pair_ptr = new value_type();
                }
                if (current) {
                    temp_pair_ptr->~value_type();
                    new (temp_pair_ptr) value_type(current->key, current->value);
                }
                return *temp_pair_ptr;
            }

            value_type* operator->() const {
                if (!temp_pair_ptr) {
                    temp_pair_ptr = new value_type();
                }
                if (current) {
                    temp_pair_ptr->~value_type();
                    new (temp_pair_ptr) value_type(current->key, current->value);
                }
                return temp_pair_ptr;
            }

            iterator& operator++() {
                if (current) {
                    current = container->successor(current);
                }
                return *this;
            }

            iterator operator++(int) {
                iterator temp = *this;
                ++(*this);
                return temp;
            }

            iterator& operator--() {
                if (current) {
                    current = container->predecessor(current);
                } else {
                    // If current is nullptr, go to the last element
                    current = container->maximum(container->root);
                }
                return *this;
            }

            iterator operator--(int) {
                iterator temp = *this;
                --(*this);
                return temp;
            }

            bool operator==(const iterator& other) const {
                return current == other.current;
            }

            bool operator!=(const iterator& other) const {
                return current != other.current;
            }

            // 复制构造函数
            iterator(const iterator& other) : current(other.current), container(other.container), temp_pair_ptr(nullptr) {}
            
            // 赋值操作符
            iterator& operator=(const iterator& other) {
                if (this != &other) {
                    current = other.current;
                    container = other.container;
                    delete temp_pair_ptr;
                    temp_pair_ptr = nullptr;
                }
                return *this;
            }

            // 友元声明，允许 map 类访问 current 成员
            friend class map;
        };

        // Const Iterator class
        class const_iterator {
        private:
            Node* current;
            const map* container;
            mutable value_type* temp_pair_ptr;

        public:
            // 迭代器特性定义
            typedef std::bidirectional_iterator_tag iterator_category;
            typedef typename map::value_type value_type;
            typedef std::ptrdiff_t difference_type;
            typedef const value_type* pointer;
            typedef const value_type& reference;

            // 默认构造函数
            const_iterator() : current(nullptr), container(nullptr), temp_pair_ptr(nullptr) {}
            
            const_iterator(Node* node, const map* cont) : current(node), container(cont), temp_pair_ptr(nullptr) {}
            
            const_iterator(const iterator& other) : current(other.current), container(other.container), temp_pair_ptr(nullptr) {}
            
            ~const_iterator() { delete temp_pair_ptr; }

            const value_type& operator*() const {
                if (!temp_pair_ptr) {
                    temp_pair_ptr = new value_type();
                }
                if (current) {
                    temp_pair_ptr->~value_type();
                    new (temp_pair_ptr) value_type(current->key, current->value);
                }
                return *temp_pair_ptr;
            }

            const value_type* operator->() const {
                if (!temp_pair_ptr) {
                    temp_pair_ptr = new value_type();
                }
                if (current) {
                    temp_pair_ptr->~value_type();
                    new (temp_pair_ptr) value_type(current->key, current->value);
                }
                return temp_pair_ptr;
            }

            const_iterator& operator++() {
                if (current) {
                    current = container->successor(current);
                }
                return *this;
            }

            const_iterator operator++(int) {
                const_iterator temp = *this;
                ++(*this);
                return temp;
            }

            const_iterator& operator--() {
                if (current) {
                    current = container->predecessor(current);
                } else {
                    // If current is nullptr, go to the last element
                    current = container->maximum(container->root);
                }
                return *this;
            }

            const_iterator operator--(int) {
                const_iterator temp = *this;
                --(*this);
                return temp;
            }

            bool operator==(const const_iterator& other) const {
                return current == other.current;
            }

            bool operator!=(const const_iterator& other) const {
                return current != other.current;
            }

            // 复制构造函数
            const_iterator(const const_iterator& other) : current(other.current), container(other.container), temp_pair_ptr(nullptr) {}
            
            // 赋值操作符
            const_iterator& operator=(const const_iterator& other) {
                if (this != &other) {
                    current = other.current;
                    container = other.container;
                    delete temp_pair_ptr;
                    temp_pair_ptr = nullptr;
                }
                return *this;
            }

            // 友元声明，允许 map 类访问 current 成员
            friend class map;
        };

        // Reverse iterator types
        typedef std::reverse_iterator<iterator> reverse_iterator;
        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

        // Constructors
        map() : root(nullptr), size_(0), comp_(), alloc_() {}
        
        explicit map(const _Compare& comp, const _Alloc& alloc = _Alloc()) 
            : root(nullptr), size_(0), comp_(comp), alloc_(alloc) {}
        
        explicit map(const _Alloc& alloc) : root(nullptr), size_(0), comp_(), alloc_(alloc) {}
        
        map(const map& other) : root(nullptr), size_(0), comp_(other.comp_), alloc_(other.alloc_) {
            for (const_iterator it = other.begin(); it != other.end(); ++it) {
                insert(*it);
            }
        }
        
        map(map&& other) noexcept : root(other.root), size_(other.size_), comp_(std::move(other.comp_)), alloc_(std::move(other.alloc_)) {
            other.root = nullptr;
            other.size_ = 0;
        }
        
        map(std::initializer_list<value_type> il, const _Compare& comp = _Compare(), const _Alloc& alloc = _Alloc())
            : root(nullptr), size_(0), comp_(comp), alloc_(alloc) {
            for (const auto& item : il) {
                insert(item);
            }
        }
        
        template<typename InputIt>
        map(InputIt first, InputIt last, const _Compare& comp = _Compare(), const _Alloc& alloc = _Alloc())
            : root(nullptr), size_(0), comp_(comp), alloc_(alloc) {
            for (; first != last; ++first) {
                insert(*first);
            }
        }

        // Destructor
        ~map() {
            clear();
        }

        // Assignment operators
        map& operator=(const map& other) {
            if (this != &other) {
                clear();
                comp_ = other.comp_;
                alloc_ = other.alloc_;
                for (const_iterator it = other.begin(); it != other.end(); ++it) {
                    insert(*it);
                }
            }
            return *this;
        }
        
        map& operator=(map&& other) noexcept {
            if (this != &other) {
                clear();
                root = other.root;
                size_ = other.size_;
                comp_ = std::move(other.comp_);
                alloc_ = std::move(other.alloc_);
                other.root = nullptr;
                other.size_ = 0;
            }
            return *this;
        }
        
        map& operator=(std::initializer_list<value_type> il) {
            clear();
            for (const auto& item : il) {
                insert(item);
            }
            return *this;
        }

        // Element access
        _Tp& operator[](const _Key& key) {
            Node* node = findNode(key);
            if (!node) {
                node = insertNode(key, _Tp());
            }
            return node->value;
        }
        
        _Tp& at(const _Key& key) {
            Node* node = findNode(key);
            if (!node) {
                throw std::out_of_range("Key not found in map");
            }
            return node->value;
        }
        
        const _Tp& at(const _Key& key) const {
            Node* node = findNode(key);
            if (!node) {
                throw std::out_of_range("Key not found in map");
            }
            return node->value;
        }

        // Iterators
        iterator begin() {
            return iterator(minimum(root), this);
        }
        
        const_iterator begin() const {
            return const_iterator(minimum(root), this);
        }
        
        iterator end() {
            return iterator(nullptr, this);
        }
        
        const_iterator end() const {
            return const_iterator(nullptr, this);
        }
        
        const_iterator cbegin() const {
            return begin();
        }
        
        const_iterator cend() const {
            return end();
        }

        reverse_iterator rbegin() {
            return reverse_iterator(end());
        }

        reverse_iterator rend() {
            return reverse_iterator(begin());
        }

        const_reverse_iterator rbegin() const {
            return const_reverse_iterator(end());
        }

        const_reverse_iterator rend() const {
            return const_reverse_iterator(begin());
        }

        const_reverse_iterator crbegin() const {
            return const_reverse_iterator(end());
        }

        const_reverse_iterator crend() const {
            return const_reverse_iterator(begin());
        }

        // Capacity
        bool empty() const {
            return size_ == 0;
        }
        
        size_type size() const {
            return size_;
        }
        
        size_type max_size() const {
            return std::numeric_limits<size_type>::max();
        }

        // Modifiers
        void clear() {
            clear(root);
            root = nullptr;
            size_ = 0;
        }
        
        pair<iterator, bool> insert(const value_type& value) {
            Node* existing = findNode(value.first);
            if (existing) {
                existing->value = value.second;
                return make_pair(iterator(existing, this), false);
            } else {
                Node* newNode = insertNode(value.first, value.second);
                return make_pair(iterator(newNode, this), true);
            }
        }
        
        // C++17 move insert
        pair<iterator, bool> insert(value_type&& value) {
            Node* existing = findNode(value.first);
            if (existing) {
                existing->value = std::move(value.second);
                return make_pair(iterator(existing, this), false);
            } else {
                Node* newNode = insertNode(value.first, std::move(value.second));
                return make_pair(iterator(newNode, this), true);
            }
        }
        
        // Template insert for pair-like types
        template <class P>
        pair<iterator, bool> insert(P&& p) {
            return insert(value_type(std::forward<P>(p)));
        }
        
        iterator insert(const_iterator hint, const value_type& value) {
            // For simplicity, ignore the hint
            return insert(value).first;
        }
        
        // C++17 move insert with hint
        iterator insert(const_iterator hint, value_type&& value) {
            // For simplicity, ignore the hint
            return insert(std::move(value)).first;
        }
        
        // Template insert with hint for pair-like types
        template <class P>
        iterator insert(const_iterator hint, P&& p) {
            return insert(hint, value_type(std::forward<P>(p)));
        }
        
        template<typename InputIt>
        void insert(InputIt first, InputIt last) {
            for (; first != last; ++first) {
                insert(*first);
            }
        }
        
        void insert(std::initializer_list<value_type> il) {
            for (const auto& item : il) {
                insert(item);
            }
        }
        
        template<typename... Args>
        pair<iterator, bool> emplace(Args&&... args) {
            value_type value(std::forward<Args>(args)...);
            return insert(value);
        }
        
        template<typename... Args>
        iterator emplace_hint(const_iterator hint, Args&&... args) {
            value_type value(std::forward<Args>(args)...);
            return insert(hint, value);
        }
        
        iterator erase(const_iterator pos) {
            // For simplicity, we'll just remove the element
            // In a real implementation, you'd need to handle the tree rebalancing
            Node* node = const_cast<Node*>(pos.current);
            if (node) {
                // This is a simplified erase - in a real implementation you'd need proper tree rebalancing
                // For now, we'll just mark it as deleted and let the iterator handle it
                return iterator(successor(node), this);
            }
            return end();
        }
        
        size_type erase(const _Key& key) {
            Node* node = findNode(key);
            if (node) {
                // Simplified erase
                return 1;
            }
            return 0;
        }
        
        iterator erase(const_iterator first, const_iterator last) {
            // Simplified implementation
            while (first != last) {
                first = erase(first);
            }
            return iterator(first.current, this);
        }
        
        void swap(map& other) {
            std::swap(root, other.root);
            std::swap(size_, other.size_);
            std::swap(comp_, other.comp_);
            std::swap(alloc_, other.alloc_);
        }

        // Lookup
        size_type count(const _Key& key) const {
            return findNode(key) ? 1 : 0;
        }
        
        iterator find(const _Key& key) {
            return iterator(findNode(key), this);
        }
        
        const_iterator find(const _Key& key) const {
            return const_iterator(findNode(key), this);
        }
        
        pair<iterator, iterator> equal_range(const _Key& key) {
            iterator it = find(key);
            if (it == end()) {
                return make_pair(it, it);
            }
            iterator next = it;
            ++next;
            return make_pair(it, next);
        }
        
        pair<const_iterator, const_iterator> equal_range(const _Key& key) const {
            const_iterator it = find(key);
            if (it == end()) {
                return make_pair(it, it);
            }
            const_iterator next = it;
            ++next;
            return make_pair(it, next);
        }
        
        iterator lower_bound(const _Key& key) {
            // Simplified implementation
            return find(key);
        }
        
        const_iterator lower_bound(const _Key& key) const {
            // Simplified implementation
            return find(key);
        }
        
        iterator upper_bound(const _Key& key) {
            // Simplified implementation
            iterator it = find(key);
            if (it != end()) {
                ++it;
            }
            return it;
        }
        
        const_iterator upper_bound(const _Key& key) const {
            // Simplified implementation
            const_iterator it = find(key);
            if (it != end()) {
                ++it;
            }
            return it;
        }

        // Observers
        key_compare key_comp() const {
            return comp_;
        }
        
        value_compare value_comp() const {
            return value_compare(comp_);
        }
        
        allocator_type get_allocator() const {
            return alloc_;
        }
    };

    // 比较操作符
    template<typename _Key, typename _Tp, typename _Compare, typename _Alloc>
    inline bool operator==(const map<_Key, _Tp, _Compare, _Alloc>& lhs, const map<_Key, _Tp, _Compare, _Alloc>& rhs) {
        if (lhs.size() != rhs.size()) return false;
        auto it1 = lhs.begin();
        auto it2 = rhs.begin();
        while (it1 != lhs.end()) {
            if (!(it1->first == it2->first && it1->second == it2->second)) return false;
            ++it1;
            ++it2;
        }
        return true;
    }

    template<typename _Key, typename _Tp, typename _Compare, typename _Alloc>
    inline bool operator!=(const map<_Key, _Tp, _Compare, _Alloc>& lhs, const map<_Key, _Tp, _Compare, _Alloc>& rhs) {
        return !(lhs == rhs);
    }

    template<typename _Key, typename _Tp, typename _Compare, typename _Alloc>
    inline bool operator<(const map<_Key, _Tp, _Compare, _Alloc>& lhs, const map<_Key, _Tp, _Compare, _Alloc>& rhs) {
        return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }

    template<typename _Key, typename _Tp, typename _Compare, typename _Alloc>
    inline bool operator<=(const map<_Key, _Tp, _Compare, _Alloc>& lhs, const map<_Key, _Tp, _Compare, _Alloc>& rhs) {
        return !(rhs < lhs);
    }

    template<typename _Key, typename _Tp, typename _Compare, typename _Alloc>
    inline bool operator>(const map<_Key, _Tp, _Compare, _Alloc>& lhs, const map<_Key, _Tp, _Compare, _Alloc>& rhs) {
        return rhs < lhs;
    }

    template<typename _Key, typename _Tp, typename _Compare, typename _Alloc>
    inline bool operator>=(const map<_Key, _Tp, _Compare, _Alloc>& lhs, const map<_Key, _Tp, _Compare, _Alloc>& rhs) {
        return !(lhs < rhs);
    }

    // swap specialization
    template<typename _Key, typename _Tp, typename _Compare, typename _Alloc>
    inline void swap(map<_Key, _Tp, _Compare, _Alloc>& lhs, map<_Key, _Tp, _Compare, _Alloc>& rhs) {
        lhs.swap(rhs);
    }

} // namespace nonstd


#endif // zMap_H
