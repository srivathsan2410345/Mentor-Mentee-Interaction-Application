#include <cstdio>  
#include <cstring> 
#include <stdlib.h>
#include "meetings.h" 

void Insert(struct meetings** head, const char* mentee, struct meetSlot ms) {
    struct meetings* newNode = (struct meetings*)malloc(sizeof(struct meetings));
    if (!newNode) {
        printf("Memory allocation failed\n");
        return;
    }

    strcpy(newNode->mentee, mentee);
    newNode->MS = ms;
    newNode->next = NULL;

    // If list is empty or new date is earlier than head
    if (*head == NULL || 
        (ms.year < (*head)->MS.year) || 
        (ms.year == (*head)->MS.year && ms.month < (*head)->MS.month) ||
        (ms.year == (*head)->MS.year && ms.month == (*head)->MS.month && ms.day < (*head)->MS.day) ||
        (ms.year == (*head)->MS.year && ms.month == (*head)->MS.month && ms.day == (*head)->MS.day && ms.slot < (*head)->MS.slot)) {
        newNode->next = *head;
        *head = newNode;
        return;
    }

    // Find position to insert (sorted by date, then by slot)
    struct meetings* current = *head;
    while (current->next != NULL && 
          (current->next->MS.year < ms.year || 
          (current->next->MS.year == ms.year && current->next->MS.month < ms.month) ||
          (current->next->MS.year == ms.year && current->next->MS.month == ms.month && current->next->MS.day < ms.day) ||
          (current->next->MS.year == ms.year && current->next->MS.month == ms.month && current->next->MS.day == ms.day && current->next->MS.slot < ms.slot))) {
        current = current->next;
    }

    // Insert after current
    newNode->next = current->next;
    current->next = newNode;
}

void LoadFromCSV(struct meetings** head, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("No existing meetings file or unable to open for reading.\n");
        return;
    }

    char line[256];
    // Skip header if it exists
    if (fgets(line, sizeof(line), file) == NULL) {
        fclose(file);
        return;
    }

    // Check if first line is header or data
    if (strstr(line, "mentee") != NULL || strstr(line, "Mentee") != NULL) {
        // This was a header, continue reading
    } else {
        // This was data, rewind to start of file
        rewind(file);
    }

    // Read all meetings
    while (fgets(line, sizeof(line), file)) {
        char mentee[100];
        struct meetSlot MS;
        int bookedValue;

        if (sscanf(line, "%99[^,],%c,%d,%d,%d,%d", mentee, &MS.slot, &MS.day, &MS.month, &MS.year, &bookedValue) == 6) {
            MS.booked = (bookedValue == 1);
            Insert(head, mentee, MS);
        }
    }

    fclose(file);
}

void PrintAvailableMeetings(struct meetings* head) {
    struct meetings* current = head;
    while (current != NULL) {
        if (!current->MS.booked) { // If meeting is not booked
            printf("%c,%d,%d,%d\n", current->MS.slot, current->MS.day, current->MS.month, current->MS.year);
        }
        current = current->next;
    }
}

int main() {
    struct meetings* head = NULL; 

    LoadFromCSV(&head, "meetings/meetings.csv");

    PrintAvailableMeetings(head); 

    return 0;
}
