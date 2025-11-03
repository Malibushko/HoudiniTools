#pragma once
#include <algorithm>
#include <memory>
#include <optional>
#include <stack>

template <
    typename TSegment,
    typename Comparator,
    typename Allocator = std::allocator<TSegment>
>
class AVLTree {
private:
    struct Node {
        TSegment data;
        Node* left;
        Node* right;
        int height;

        Node(const TSegment& val) : data(val), left(nullptr), right(nullptr), height(1) {}
    };

    using NodeAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;

    Node* root;
    Comparator comparator;
    NodeAllocator node_alloc;

    int height(Node* n) { return n ? n->height : 0; }
    int get_balance(Node* n) { return n ? height(n->left) - height(n->right) : 0; }

    Node* right_rotate(Node* y) {
        Node* x = y->left;
        Node* T2 = x->right;
        x->right = y;
        y->left = T2;
        y->height = std::max(height(y->left), height(y->right)) + 1;
        x->height = std::max(height(x->left), height(x->right)) + 1;
        return x;
    }

    Node* left_rotate(Node* x) {
        Node* y = x->right;
        Node* T2 = y->left;
        y->left = x;
        x->right = T2;
        x->height = std::max(height(x->left), height(x->right)) + 1;
        y->height = std::max(height(y->left), height(y->right)) + 1;
        return y;
    }


    Node* find_min_value_node(Node* node) {
        Node* current = node;
        while (current && current->left)
            current = current->left;
        return current;
    }

    Node* find_node(Node* node, const TSegment& segment) {
        if (!node) return nullptr;
        if (!comparator(segment, node->data) && !comparator(node->data, segment)) return node;
        if (comparator(segment, node->data))
            return find_node(node->left, segment);
        else
            return find_node(node->right, segment);
    }

    Node* insert_recursive(Node* node, const TSegment& segment) {
        if (!node) {
            Node* newNode = node_alloc.allocate(1);
            std::allocator_traits<NodeAllocator>::construct(node_alloc, newNode, segment);
            return newNode;
        }

        if (comparator(segment, node->data))
            node->left = insert_recursive(node->left, segment);
        else if (comparator(node->data, segment))
            node->right = insert_recursive(node->right, segment);
        else
            return node;

        node->height = 1 + std::max(height(node->left), height(node->right));
        int balance = get_balance(node);

        if (balance > 1 && comparator(segment, node->left->data)) return right_rotate(node);
        if (balance < -1 && comparator(node->right->data, segment)) return left_rotate(node);
        if (balance > 1 && comparator(node->left->data, segment)) {
            node->left = left_rotate(node->left);
            return right_rotate(node);
        }
        if (balance < -1 && comparator(segment, node->right->data)) {
            node->right = right_rotate(node->right);
            return left_rotate(node);
        }
        return node;
    }

    Node* remove_recursive(Node* node, const TSegment& segment) {
        if (!node) return node;

        if (comparator(segment, node->data))
            node->left = remove_recursive(node->left, segment);
        else if (comparator(node->data, segment))
            node->right = remove_recursive(node->right, segment);
        else {
            if (!node->left || !node->right) {
                Node* temp = node->left ? node->left : node->right;
                if (!temp) {
                    temp = node;
                    node = nullptr;
                } else {
                    *node = *temp;
                }
                std::allocator_traits<NodeAllocator>::destroy(node_alloc, temp);
                node_alloc.deallocate(temp, 1);
            } else {
                Node* temp = find_min_value_node(node->right);
                node->data = temp->data;
                node->right = remove_recursive(node->right, temp->data);
            }
        }

        if (!node) return node;

        node->height = 1 + std::max(height(node->left), height(node->right));
        int balance = get_balance(node);

        if (balance > 1 && get_balance(node->left) >= 0)
            return right_rotate(node);
        if (balance > 1 && get_balance(node->left) < 0) {
            node->left = left_rotate(node->left);
            return right_rotate(node);
        }
        if (balance < -1 && get_balance(node->right) <= 0)
            return left_rotate(node);
        if (balance < -1 && get_balance(node->right) > 0) {
            node->right = right_rotate(node->right);
            return left_rotate(node);
        }

        return node;
    }

    void destroy_recursive(Node* node) {
        if (node) {
            destroy_recursive(node->left);
            destroy_recursive(node->right);
            std::allocator_traits<NodeAllocator>::destroy(node_alloc, node);
            node_alloc.deallocate(node, 1);
        }
    }
    void inorder_collect(Node* node, std::vector<TSegment>& elements) {
        if (!node) return;
        inorder_collect(node->left, elements);
        elements.push_back(node->data);
        inorder_collect(node->right, elements);
    }

    size_t size_estimate(Node* node) const {
        if (!node) return 0;
        return 1 + size_estimate(node->left) + size_estimate(node->right);
    }

    Node* build_balanced(const std::vector<TSegment>& elements, int start, int end) {
        if (start > end) return nullptr;

        int mid = start + (end - start) / 2;
        Node* node = node_alloc.allocate(1);
        std::allocator_traits<NodeAllocator>::construct(node_alloc, node, elements[mid]);

        node->left = build_balanced(elements, start, mid - 1);
        node->right = build_balanced(elements, mid + 1, end);
        node->height = 1 + std::max(height(node->left), height(node->right));

        return node;
    }

public:
    class iterator {
        std::stack<Node*> stack;

        void push_left(Node* node) {
            while (node) {
                stack.push(node);
                node = node->left;
            }
        }

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = TSegment;
        using difference_type = std::ptrdiff_t;
        using pointer = TSegment*;
        using reference = TSegment&;

        iterator() = default;
        explicit iterator(Node* root) { push_left(root); }

        reference operator*() const { return stack.top()->data; }
        pointer operator->() const { return &stack.top()->data; }

        iterator& operator++() {
            Node* node = stack.top();
            stack.pop();
            if (node->right)
                push_left(node->right);
            return *this;
        }

        iterator operator++(int) {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const iterator& other) const { return stack == other.stack; }
        bool operator!=(const iterator& other) const { return !(*this == other); }
    };

    iterator begin() const { return iterator(root); }
    iterator end() const { return iterator(); }

    // ----- MAIN INTERFACE -----
    explicit AVLTree(const Comparator& comp, const Allocator& alloc = Allocator())
        : root(nullptr), comparator(comp), node_alloc(alloc) {}

    ~AVLTree() { destroy_recursive(root); }

    AVLTree(const AVLTree&) = delete;
    AVLTree& operator=(const AVLTree&) = delete;

    void insert(const TSegment& segment) { root = insert_recursive(root, segment); }
    void remove(const TSegment& segment) { root = remove_recursive(root, segment); }

    bool swap(const TSegment& segment1, const TSegment& segment2) {
        Node* node1 = find_node(root, segment1);
        Node* node2 = find_node(root, segment2);
        if (!node1 || !node2) return false;
        std::swap(node1->data, node2->data);
        return true;
    }

    void rebuild() {
        std::vector<TSegment> elements;
        elements.reserve(size_estimate(root));
        inorder_collect(root, elements);

        destroy_recursive(root);
        root = nullptr;

        root = build_balanced(elements, 0, static_cast<int>(elements.size()) - 1);
    }

    std::optional<TSegment> find_successor(const TSegment& segment) const {
        Node* current = root;
        Node* successor = nullptr;
        while (current) {
            if (comparator(segment, current->data)) {
                successor = current;
                current = current->left;
            } else
                current = current->right;
        }
        if (successor) return successor->data;
        return std::nullopt;
    }

    std::optional<TSegment> find_predecessor(const TSegment& segment) const {
        Node* current = root;
        Node* predecessor = nullptr;
        while (current) {
            if (comparator(current->data, segment)) {
                predecessor = current;
                current = current->right;
            } else
                current = current->left;
        }
        if (predecessor) return predecessor->data;
        return std::nullopt;
    }
};
