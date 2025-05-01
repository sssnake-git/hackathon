/* red_black_tree.c */
#include <stdio.h>
#include <stdlib.h>

typedef enum { RED, BLACK } Color;

typedef struct red_black_node {
    int key;
    Color color;
    struct red_black_node *left, *right, *parent;
} red_black_node;

typedef struct {
    red_black_node *root;
    red_black_node *nil;
} red_black_tree;

red_black_tree *create_tree();
red_black_node *create_node(red_black_tree *tree, int key);
void left_rotate(red_black_tree *tree, red_black_node *x);
void right_rotate(red_black_tree *tree, red_black_node *y);
void insert(red_black_tree *tree, int key);
void insert_fixup(red_black_tree *tree, red_black_node *z);
void inorder(red_black_tree *tree, red_black_node *node);
void destroy_tree(red_black_tree *tree, red_black_node *node);

red_black_tree *create_tree() {
    red_black_tree *tree = malloc(sizeof(red_black_tree));
    tree->nil = malloc(sizeof(red_black_node));
    tree->nil->color = BLACK;
    tree->nil->left = tree->nil->right = tree->nil->parent = NULL;
    tree->root = tree->nil;
    return tree;
}

red_black_node *create_node(red_black_tree *tree, int key) {
    red_black_node *node = malloc(sizeof(red_black_node));
    node->key = key;
    node->color = RED;
    node->left = node->right = node->parent = tree->nil;
    return node;
}

void left_rotate(red_black_tree *tree, red_black_node *x) {
    red_black_node *y = x->right;
    x->right = y->left;
    if (y->left != tree->nil)
        y->left->parent = x;
    y->parent = x->parent;
    if (x->parent == tree->nil)
        tree->root = y;
    else if (x == x->parent->left)
        x->parent->left = y;
    else
        x->parent->right = y;
    y->left = x;
    x->parent = y;
}

void right_rotate(red_black_tree *tree, red_black_node *y) {
    red_black_node *x = y->left;
    y->left = x->right;
    if (x->right != tree->nil)
        x->right->parent = y;
    x->parent = y->parent;
    if (y->parent == tree->nil)
        tree->root = x;
    else if (y == y->parent->right)
        y->parent->right = x;
    else
        y->parent->left = x;
    x->right = y;
    y->parent = x;
}

void insert(red_black_tree *tree, int key) {
    red_black_node *z = create_node(tree, key);
    red_black_node *y = tree->nil;
    red_black_node *x = tree->root;

    while (x != tree->nil) {
        y = x;
        if (z->key < x->key)
            x = x->left;
        else
            x = x->right;
    }
    z->parent = y;
    if (y == tree->nil)
        tree->root = z;
    else if (z->key < y->key)
        y->left = z;
    else
        y->right = z;
    z->left = z->right = tree->nil;
    z->color = RED;
    insert_fixup(tree, z);
}

void insert_fixup(red_black_tree *tree, red_black_node *z) {
    while (z->parent->color == RED) {
        if (z->parent == z->parent->parent->left) {
            red_black_node *y = z->parent->parent->right;
            if (y->color == RED) {
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->right) {
                    z = z->parent;
                    left_rotate(tree, z);
                }
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                right_rotate(tree, z->parent->parent);
            }
        } else {
            red_black_node *y = z->parent->parent->left;
            if (y->color == RED) {
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->left) {
                    z = z->parent;
                    right_rotate(tree, z);
                }
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                left_rotate(tree, z->parent->parent);
            }
        }
    }
    tree->root->color = BLACK;
}

void inorder(red_black_tree *tree, red_black_node *node) {
    if (node != tree->nil) {
        inorder(tree, node->left);
        printf("%d ", node->key);
        inorder(tree, node->right);
    }
}

void destroy_tree(red_black_tree *tree, red_black_node *node) {
    if (node != tree->nil) {
        destroy_tree(tree, node->left);
        destroy_tree(tree, node->right);
        free(node);
    }
}
