#include "red_black_tree.c"

void run_test_case(const char* name, int* data, int n) {
    printf("%s\n", name);
    red_black_tree *tree = create_tree();
    for (int i = 0; i < n; i++) {
        insert(tree, data[i]);
    }
    inorder(tree, tree->root);
    printf("\n\n");
    destroy_tree(tree, tree->root);
    free(tree->nil);
    free(tree);
}

int main() {
    int data1[] = {10, 20, 30, 15, 25, 5};
    int data2[] = {50, 40, 30, 20, 10};
    int data3[] = {100, 150, 120, 130, 80, 60, 70};
    int data4[] = {5, 4, 3, 2, 1}; // Worst-case descending
    int data5[] = {1, 2, 3, 4, 5}; // Worst-case ascending

    run_test_case("Test Case 1", data1, sizeof(data1)/sizeof(data1[0]));
    run_test_case("Test Case 2", data2, sizeof(data2)/sizeof(data2[0]));
    run_test_case("Test Case 3", data3, sizeof(data3)/sizeof(data3[0]));
    run_test_case("Test Case 4 (descending)", data4, sizeof(data4)/sizeof(data4[0]));
    run_test_case("Test Case 5 (ascending)", data5, sizeof(data5)/sizeof(data5[0]));

    return 0;
}
