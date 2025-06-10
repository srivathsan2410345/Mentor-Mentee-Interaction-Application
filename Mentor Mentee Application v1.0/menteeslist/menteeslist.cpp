#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME_LENGTH 100
#define MAX_DEPT_LENGTH 100
#define FILE_NAME "menteeslist/menteeslist.csv"  // Hardcoded filename

// Student structure
typedef struct {
    int roll_no;
    char name[MAX_NAME_LENGTH];
    char department[MAX_DEPT_LENGTH];
} Student;

// BST node structure
typedef struct TreeNode {
    Student data;
    struct TreeNode* left;
    struct TreeNode* right;
} TreeNode;

// Create a new BST node
TreeNode* createNode(int roll_no, const char* name, const char* department) {
    TreeNode* node = (TreeNode*)malloc(sizeof(TreeNode));
    node->data.roll_no = roll_no;
    strncpy(node->data.name, name, MAX_NAME_LENGTH);
    strncpy(node->data.department, department, MAX_DEPT_LENGTH);
    node->left = node->right = NULL;
    return node;
}

// Insert into BST
TreeNode* insert(TreeNode* root, int roll_no, const char* name, const char* department) {
    if (root == NULL) {
        return createNode(roll_no, name, department);
    }
    if (roll_no < root->data.roll_no) {
        root->left = insert(root->left, roll_no, name, department);
    } else if (roll_no > root->data.roll_no) {
        root->right = insert(root->right, roll_no, name, department);
    } else {
        printf("Student with roll number %d already exists.\n", roll_no);
    }
    return root;
}

// Search by roll number
TreeNode* search(TreeNode* root, int roll_no) {
    if (root == NULL || root->data.roll_no == roll_no) {
        return root;
    }
    if (roll_no < root->data.roll_no) {
        return search(root->left, roll_no);
    } else {
        return search(root->right, roll_no);
    }
}

// Write BST to file (in-order)
void writeToFile(TreeNode* root, FILE* file) {
    if (root == NULL) return;
    writeToFile(root->left, file);
    fprintf(file, "%d,%s,%s\n", root->data.roll_no, root->data.name, root->data.department);
    writeToFile(root->right, file);
}

// Save BST to CSV (hardcoded filename)
void saveToCSV(TreeNode* root) {
    FILE* file = fopen(FILE_NAME, "w");
    if (file == NULL) {
        printf("Could not open file to save.\n");
        return;
    }
    writeToFile(root, file);
    fclose(file);
    printf("Data saved to %s\n", FILE_NAME);
}

// Load CSV and build tree (hardcoded filename)
TreeNode* loadFromCSV() {
    FILE* file = fopen(FILE_NAME, "r");
    if (file == NULL) {
        printf("No existing file found. Starting with empty list.\n");
        return NULL;
    }

    TreeNode* root = NULL;
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        int roll_no;
        char name[MAX_NAME_LENGTH], dept[MAX_DEPT_LENGTH];
        if (sscanf(line, "%d,%99[^,],%99[^\n]", &roll_no, name, dept) == 3) {
            root = insert(root, roll_no, name, dept);
        }
    }
    fclose(file);
    printf("Data loaded from %s\n", FILE_NAME);
    return root;
}

// Find the minimum value node in the tree
TreeNode* findMin(TreeNode* root) {
    while (root && root->left != NULL) {
        root = root->left;
    }
    return root;
}

// Delete a node from the BST
TreeNode* deleteNode(TreeNode* root, int roll_no) {
    if (root == NULL) {
        printf("Student with roll number %d not found.\n", roll_no);
        return root;
    }

    if (roll_no < root->data.roll_no) {
        root->left = deleteNode(root->left, roll_no);
    } else if (roll_no > root->data.roll_no) {
        root->right = deleteNode(root->right, roll_no);
    } else {
        // Node with one child or no child
        if (root->left == NULL) {
            TreeNode* temp = root->right;
            free(root);
            return temp;
        } else if (root->right == NULL) {
            TreeNode* temp = root->left;
            free(root);
            return temp;
        }

        // Node with two children: Get the in-order successor (smallest in the right subtree)
        TreeNode* temp = findMin(root->right);

        // Copy the in-order successor's content to this node
        root->data = temp->data;

        // Delete the in-order successor
        root->right = deleteNode(root->right, temp->data.roll_no);
    }

    return root;
}

// Free memory
void deleteTree(TreeNode* root) {
    if (root == NULL) return;
    deleteTree(root->left);
    deleteTree(root->right);
    free(root);
}

// Main program to interact with command-line arguments
int main(int argc, char *argv[]) {
    TreeNode* root = loadFromCSV();
    
    if (argc < 2) {
        printf("Usage: ./bst [command] [arguments]\n");
        return 1;
    }

    if (strcmp(argv[1], "load") == 0) {
                // In-order print
        if (root == NULL) {
            printf("No mentees found.\n");
        } else {
            writeToFile(root, stdout);  // Print CSV lines to stdout
        }
        
        
    } else if (strcmp(argv[1], "insert") == 0 && argc == 5) {
        int roll_no = atoi(argv[2]);
        const char* name = argv[3];
        const char* department = argv[4];

        root = insert(root, roll_no, name, department);
        saveToCSV(root);  // Save after insertion
        printf("Student inserted.\n");
        
        
    } else if (strcmp(argv[1], "search") == 0 && argc == 3) {
        int roll_no = atoi(argv[2]);
        TreeNode* found = search(root, roll_no);
        if (found) {
            printf("Found: %d | %s | %s\n", found->data.roll_no, found->data.name, found->data.department);
        } else {
            printf("Student not found.\n");
        }
        
        
    } else if (strcmp(argv[1], "delete") == 0 && argc == 3) {
        int roll_no = atoi(argv[2]);
        root = deleteNode(root, roll_no);  // Delete student
        saveToCSV(root);  // Save after deletion
        printf("Student deleted.\n");
    }

    else if (strcmp(argv[1], "save") == 0) {
        saveToCSV(root);
    }

    // Clean up
    deleteTree(root);
    return 0;
}
