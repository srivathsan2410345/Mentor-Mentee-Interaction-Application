#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TASKS 1000
#define MAX_TITLE_LENGTH 128
#define MAX_DESC_LENGTH 512
#define MAX_DATE_LENGTH 20
#define MAX_PRIORITY_LENGTH 10
#define MAX_MENTEE_LENGTH 50

typedef struct {
    char mentee_name[MAX_MENTEE_LENGTH];
    char title[MAX_TITLE_LENGTH];
    char description[MAX_DESC_LENGTH];
    char due_date[MAX_DATE_LENGTH];
    int priority;
} Task;

typedef struct {
    Task tasks[MAX_TASKS];
    int size;
} MinHeap;

void initializeHeap(MinHeap* heap) {
    heap->size = 0;
}

void swap(Task* a, Task* b) {
    Task temp = *a;
    *a = *b;
    *b = temp;
}

int compareTasks(const Task* a, const Task* b) {
    int dateComparison = strcmp(a->due_date, b->due_date);
    if (dateComparison < 0) {
        return -1; // a earlier than b
    } else if (dateComparison > 0) {
        return 1;  // a later than b
    } else {
        // Dates are the same, compare priority
        if (a->priority < b->priority) return -1;
        if (a->priority > b->priority) return 1;
        return 0; // Same date and priority
    }
}

void heapifyUp(MinHeap* heap, int index) {
    if (index == 0) return;
    int parent = (index - 1) / 2;
    if (compareTasks(&heap->tasks[index], &heap->tasks[parent]) < 0) {
        swap(&heap->tasks[index], &heap->tasks[parent]);
        heapifyUp(heap, parent);
    }
}

void heapifyDown(MinHeap* heap, int index) {
    int left = 2 * index + 1;
    int right = 2 * index + 2;
    int smallest = index;

    if (left < heap->size && compareTasks(&heap->tasks[left], &heap->tasks[smallest]) < 0) {
        smallest = left;
    }
    if (right < heap->size && compareTasks(&heap->tasks[right], &heap->tasks[smallest]) < 0) {
        smallest = right;
    }
    if (smallest != index) {
        swap(&heap->tasks[index], &heap->tasks[smallest]);
        heapifyDown(heap, smallest);
    }
}

void insertTask(MinHeap* heap, const char* mentee_name, const char* title, const char* description, const char* due_date, int priority) {
    if (heap->size >= MAX_TASKS) {
        fprintf(stderr, "Heap is full\n");
        return;
    }
    Task newTask;
    strncpy(newTask.mentee_name, mentee_name, MAX_MENTEE_LENGTH);
    strncpy(newTask.title, title, MAX_TITLE_LENGTH);
    strncpy(newTask.description, description, MAX_DESC_LENGTH);
    strncpy(newTask.due_date, due_date, MAX_DATE_LENGTH);
    newTask.priority = priority;

    heap->tasks[heap->size] = newTask;
    heapifyUp(heap, heap->size);
    heap->size++;
}

Task* extractMin(MinHeap* heap) {
    if (heap->size == 0) return NULL;
    Task* min = (Task*)malloc(sizeof(Task));
    if (!min) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    *min = heap->tasks[0];
    heap->tasks[0] = heap->tasks[heap->size - 1];
    heap->size--;
    heapifyDown(heap, 0);
    return min;
}

int removeSpecific(MinHeap* heap, const char* title) {
    int found = 0;
    for (int i = 0; i < heap->size; i++) {
        if (strcmp(heap->tasks[i].title, title) == 0) {
            for (int j = i; j < heap->size - 1; j++) {
                heap->tasks[j] = heap->tasks[j + 1];
            }
            heap->size--; 
            found = 1;
            printf("Task '%s' removed.\n", title);
            break;
        }
    }
    if (!found) {
        printf("Task '%s' not found.\n", title);
        return 0;
    }
    return 1;
}

Task* peekMin(MinHeap* heap) {
    if (heap->size == 0) return NULL;
    return &heap->tasks[0];
}

void clearHeap(MinHeap* heap) {
    heap->size = 0;
}

void LoadFromCSV(MinHeap* heap, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) return;

    char line[MAX_TITLE_LENGTH + MAX_DESC_LENGTH + MAX_DATE_LENGTH + MAX_PRIORITY_LENGTH + MAX_MENTEE_LENGTH + 10];
    while (fgets(line, sizeof(line), file)) {
        char mentee_name[MAX_MENTEE_LENGTH], title[MAX_TITLE_LENGTH], description[MAX_DESC_LENGTH], due_date[MAX_DATE_LENGTH], priorityStr[MAX_PRIORITY_LENGTH];
        int priority;

        if (sscanf(line, "%[^,],%[^,],%[^,],%[^,],%s", mentee_name, title, description, due_date, priorityStr) == 5) {
            priority = atoi(priorityStr);
            insertTask(heap, mentee_name, title, description, due_date, priority);
        }
    }
    fclose(file);
}

void SaveToCSV(MinHeap* heap, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Failed to open file %s for writing\n", filename);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < heap->size; i++) {
        fprintf(file, "%s,%s,%s,%s,%d\n", 
                heap->tasks[i].mentee_name,
                heap->tasks[i].title, 
                heap->tasks[i].description, 
                heap->tasks[i].due_date, 
                heap->tasks[i].priority);
    }
    fclose(file);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <mentee_name> <command> [arguments]\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char* mentee_name = argv[1];
    const char* command = argv[2];

    MinHeap heap;
    initializeHeap(&heap);
    LoadFromCSV(&heap, "tasks/tasks.csv");

    if (strcmp(command, "insert") == 0) {
        if (argc < 7) {
            fprintf(stderr, "Usage: %s %s insert <title> <description> <due_date> <priority>\n", argv[0], mentee_name);
            clearHeap(&heap);
            return EXIT_FAILURE;
        }
        int priority = atoi(argv[6]);
        insertTask(&heap, mentee_name, argv[3], argv[4], argv[5], priority);
        printf("[%s] Task inserted: %s\n", mentee_name, argv[3]);
    }

    else if (strcmp(command, "extractMin") == 0) {
        Task* task = extractMin(&heap);
        if (task) {
            printf("[%s] Extracted Task: %s | %s | %s | %d\n", mentee_name, task->title, task->description, task->due_date, task->priority);
            free(task);
        } else {
            printf("[%s] Heap is empty\n", mentee_name);
        }
    }

    else if (strcmp(command, "removeSpecific") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: %s %s removeSpecific <title>\n", argv[0], mentee_name);
            return EXIT_FAILURE;
        }
        const char* target = argv[3];
        removeSpecific(&heap, target);
    }

    else if (strcmp(command, "peek") == 0) {
        Task* task = peekMin(&heap);
        if (task) {
            const char* P;
            switch (task->priority) {
                case 1: P = "High"; break;
                case 2: P = "Medium"; break;
                case 3: P = "Low"; break;
                default: P = "Unknown"; break;
            }
            printf("[%s] %s | %s | %s | %s\n", task->mentee_name, task->title, task->description, task->due_date, P);
        } else {
            printf("[%s] Heap is empty\n", mentee_name);
        }
    }

    else if (strcmp(command, "clear") == 0) {
        clearHeap(&heap);
        printf("[%s] Heap cleared\n", mentee_name);
    }

else if (strcmp(command, "list") == 0) {
    for (int i = 0; i < heap.size; i++) {
        const char* P;
        switch (heap.tasks[i].priority) {
            case 1: P = "High"; break;
            case 2: P = "Medium"; break;
            case 3: P = "Low"; break;
            default: P = "Unknown"; break;
        }
        printf("%s|%s|%s|%s|%s\n",
               heap.tasks[i].mentee_name,
               heap.tasks[i].title,
               heap.tasks[i].description,
               heap.tasks[i].due_date,
               P);
    }
}

    else {
        fprintf(stderr, "Unknown command: %s\n", command);
        clearHeap(&heap);
        return EXIT_FAILURE;
    }

    SaveToCSV(&heap, "tasks/tasks.csv");
    return EXIT_SUCCESS;
}
