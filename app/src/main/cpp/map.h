// map.h
#ifndef zMap_H
#define zMap_H

#include <cstddef>
#include <initializer_list>

#include "zLog.h"

namespace nonstd {
    // 使用typedef避免宏替换
    template<typename T>
    using initializer_list = std::initializer_list<T>;

    // 自定义pair实现
    template<typename T1, typename T2>
    struct pair {
        T1 first;
        T2 second;

        pair() : first(), second() {}
        pair(const T1& f, const T2& s) : first(f), second(s) {}
        pair(const pair& other) : first(other.first), second(other.second) {}

        pair& operator=(const pair& other) {
            if (this != &other) {
                first = other.first;
                second = other.second;
            }
            return *this;
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

    template<typename K, typename V>
    class map {
    private:
        struct Node {
            K key;
            V value;
            Node* left;
            Node* right;
            Node* parent;
            bool isRed;

            Node(const K& k, const V& v) : key(k), value(v), left(nullptr), right(nullptr), parent(nullptr), isRed(true) {}
        };

        Node* root;
        size_t size_;

        // Helper functions
        void clear(Node* node) {
            if (node) {
                clear(node->left);
                clear(node->right);
                delete node;
            }
        }

        Node* findNode(const K& key) const {
            Node* current = root;
            while (current) {
                if (key < current->key) {
                    current = current->left;
                } else if (key > current->key) {
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

        // Insert a new node with given key and value
        Node* insertNode(const K& key, const V& value) {
            // 使用条件编译来处理不同类型的key
            #ifdef __cpp_rtti
            if constexpr (std::is_same_v<K, string> || std::is_same_v<K, const char*>) {
                LOGD("nonstd::map: insertNode called for key='%s'", key.c_str());
            } else {
                LOGD("nonstd::map: insertNode called for key (non-string type)");
            }
            #else
            LOGD("nonstd::map: insertNode called for key");
            #endif

            Node* newNode = new Node(key, value);

            if (!root) {
                root = newNode;
                newNode->isRed = false;  // Root is always black
                LOGD("nonstd::map: insertNode - inserted as root");
            } else {
                Node* current = root;
                Node* parent = nullptr;

                // Find the position to insert
                while (current) {
                    parent = current;
                    if (key < current->key) {
                        current = current->left;
                    } else if (key > current->key) {
                        current = current->right;
                    } else {
                        // Key already exists, update value and return
                        LOGD("nonstd::map: insertNode - key already exists, updating value");
                        current->value = value;
                        delete newNode;
                        return current;
                    }
                }

                // Insert the new node
                if (key < parent->key) {
                    parent->left = newNode;
                    LOGD("nonstd::map: insertNode - inserted as left child");
                } else {
                    parent->right = newNode;
                    LOGD("nonstd::map: insertNode - inserted as right child");
                }
                newNode->parent = parent;

                // TODO: Implement Red-Black tree balancing
                // For now, just keep it as a simple BST
            }

            size_++;
            LOGD("nonstd::map: insertNode completed, new size=%zu", size_);
            return newNode;
        }

    public:
        // Iterator class
        class iterator {
        private:
            Node* current;
            const map* container;

        public:
            iterator(Node* node, const map* cont) : current(node), container(cont) {}

            pair<K, V>& operator*() {
                static pair<K, V> temp;
                if (current) {
                    temp.first = current->key;
                    temp.second = current->value;
                }
                return temp;
            }

            pair<K, V>* operator->() {
                static pair<K, V> temp;
                if (current) {
                    temp.first = current->key;
                    temp.second = current->value;
                }
                return &temp;
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

            bool operator==(const iterator& other) const {
                return current == other.current;
            }

            bool operator!=(const iterator& other) const {
                return current != other.current;
            }
        };

        // Default constructor
        map() : root(nullptr), size_(0) {
            LOGD("nonstd::map: default constructor");
        }

        // Constructor with initializer list (for brace initialization)
        map(initializer_list<pair<K, V>> init) : root(nullptr), size_(0) {
            LOGD("nonstd::map: initializer_list constructor with %zu elements", init.size());

            for (const auto& item : init) {
                insert(item);
            }

            LOGD("nonstd::map: initializer_list constructor completed, size=%zu", size_);
        }

        // Copy constructor
        map(const map& other) : root(nullptr), size_(0) {
            LOGD("nonstd::map: copy constructor from map with size=%zu", other.size_);
            
            // Copy all elements from other map
            for (const auto& item : other) {
                insert(item);
            }
            
            LOGD("nonstd::map: copy constructor completed, new size=%zu", size_);
        }

        // Move constructor
        map(map&& other) noexcept : root(other.root), size_(other.size_) {
            LOGD("nonstd::map: move constructor");
            other.root = nullptr;
            other.size_ = 0;
        }

        // Destructor
        ~map() {
            LOGD("nonstd::map: destructor, size=%zu", size_);
            clear(root);
        }

        // Copy assignment operator
        map& operator=(const map& other) {
            LOGD("nonstd::map: copy assignment from map with size=%zu", other.size_);
            if (this != &other) {
                clear(root);
                root = nullptr;
                size_ = 0;
                
                // Copy all elements from other map
                for (const auto& item : other) {
                    insert(item);
                }
            }
            LOGD("nonstd::map: copy assignment completed, new size=%zu", size_);
            return *this;
        }

        // Initializer list assignment operator
        map& operator=(initializer_list<pair<K, V>> init) {
            LOGD("nonstd::map: initializer_list assignment with %zu elements", init.size());

            clear();

            for (const auto& item : init) {
                insert(item);
            }

            LOGD("nonstd::map: initializer_list assignment completed, size=%zu", size_);
            return *this;
        }

        // Move assignment operator
        map& operator=(map&& other) noexcept {
            LOGD("nonstd::map: move assignment");
            if (this != &other) {
                clear(root);
                root = other.root;
                size_ = other.size_;
                other.root = nullptr;
                other.size_ = 0;
            }
            return *this;
        }

        // Element access
        V& operator[](const K& key) {
            LOGD("nonstd::map: operator[], key=...");
            Node* node = findNode(key);
            if (!node) {
                // Create new node if key doesn't exist
                node = insertNode(key, V());
                LOGD("nonstd::map: operator[] created new node for key, new size=%zu", size_);
            }
            return node->value;
        }

        V& at(const K& key) {
            LOGD("nonstd::map: at(), key=...");
            Node* node = findNode(key);
            if (!node) {
                static V default_value;
                return default_value;
            }
            return node->value;
        }

        const V& at(const K& key) const {
            LOGD("nonstd::map: at() const, key=...");
            Node* node = findNode(key);
            if (!node) {
                static V default_value;
                return default_value;
            }
            return node->value;
        }

        // Capacity
        bool empty() const {
            return size_ == 0;
        }

        size_t size() const {
            return size_;
        }

        // Modifiers
        pair<iterator, bool> insert(const pair<K, V>& value) {
            LOGD("nonstd::map: insert, key=..., value=...");
            
            // Check if key already exists
            Node* existing_node = findNode(value.first);
            if (existing_node) {
                LOGD("nonstd::map: insert - key already exists, updating value");
                existing_node->value = value.second;
                return make_pair(iterator(existing_node, this), false);
            }

            // Insert new node
            Node* new_node = insertNode(value.first, value.second);
            LOGD("nonstd::map: insert - new element inserted");
            
            return make_pair(iterator(new_node, this), true);
        }

        // Emplace method - construct element in place
        template<typename... Args>
        pair<iterator, bool> emplace(Args&&... args) {
            LOGD("nonstd::map: emplace called");

            // Create a pair from the arguments
            pair<K, V> new_pair(std::forward<Args>(args)...);

            // Check if key already exists
            Node* existing_node = findNode(new_pair.first);
            if (existing_node) {
                LOGD("nonstd::map: emplace - key already exists, updating value");
                existing_node->value = new_pair.second;
                return make_pair(iterator(existing_node, this), false);
            }

            // Insert new node
            Node* new_node = insertNode(new_pair.first, new_pair.second);
            LOGD("nonstd::map: emplace - new element inserted");

            return make_pair(iterator(new_node, this), true);
        }

        void erase(const K& key) {
            LOGD("nonstd::map: erase, key=...");
            // Simplified implementation
        }

        void clear() {
            LOGD("nonstd::map: clear, old size=%zu", size_);
            clear(root);
            root = nullptr;
            size_ = 0;
        }

        // Lookup
        iterator find(const K& key) {
            LOGD("nonstd::map: find, key=..., size=%zu", size_);
            Node* node = findNode(key);
            return iterator(node, this);
        }

        bool contains(const K& key) const {
            return findNode(key) != nullptr;
        }

        size_t count(const K& key) const {
            return findNode(key) ? 1 : 0;
        }

        // Iterators
        iterator begin() {
            return iterator(minimum(root), this);
        }

        iterator end() {
            return iterator(nullptr, this);
        }

        iterator begin() const {
            return iterator(minimum(root), this);
        }

        iterator end() const {
            return iterator(nullptr, this);
        }
    };






} // namespace nonstd



#endif // zMap_H 