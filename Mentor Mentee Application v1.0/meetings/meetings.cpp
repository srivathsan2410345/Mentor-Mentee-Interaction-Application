#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "meetings.h"

int isEmpty(struct meetings* front) {
    return front == NULL;
}

bool IsOverlapping(struct meetSlot newMS, struct meetSlot existingMS) {
    return (newMS.slot == existingMS.slot &&
            newMS.day == existingMS.day &&
            newMS.month == existingMS.month &&
            newMS.year == existingMS.year);
}

void Insert(struct meetings** head, const char* mentee, struct meetSlot ms) {
    struct meetings* current = *head;

    // Check for an existing meeting with the same slot, day, month, year
    while (current) {
        if (IsOverlapping(ms, current->MS)) {
            // Check if it's booked by someone else
            if (strcmp(current->mentee, "null") != 0) {
                printf("Error: Time slot already booked by another mentee\n");
                FreeMeetings(*head);
                exit(1);
            } else {
                // Overwrite existing booking
                strcpy(current->mentee, mentee);
                current->MS.booked = ms.booked;
                return;
            }
        }
        current = current->next;
    }

    // No match found, insert a new node in sorted order
    struct meetings* newNode = (struct meetings*)malloc(sizeof(struct meetings));
    if (!newNode) {
        printf("Memory allocation failed\n");
        return;
    }

    strcpy(newNode->mentee, mentee);
    newNode->MS = ms;
    newNode->next = NULL;

    // Insert at head or before first later meeting
    if (*head == NULL || 
        (ms.year < (*head)->MS.year) || 
        (ms.year == (*head)->MS.year && ms.month < (*head)->MS.month) ||
        (ms.year == (*head)->MS.year && ms.month == (*head)->MS.month && ms.day < (*head)->MS.day) ||
        (ms.year == (*head)->MS.year && ms.month == (*head)->MS.month && ms.day == (*head)->MS.day && ms.slot < (*head)->MS.slot)) {
        newNode->next = *head;
        *head = newNode;
        return;
    }

    struct meetings* prev = *head;
    while (prev->next != NULL && 
          (prev->next->MS.year < ms.year || 
          (prev->next->MS.year == ms.year && prev->next->MS.month < ms.month) ||
          (prev->next->MS.year == ms.year && prev->next->MS.month == ms.month && prev->next->MS.day < ms.day) ||
          (prev->next->MS.year == ms.year && prev->next->MS.month == ms.month && prev->next->MS.day == ms.day && prev->next->MS.slot < ms.slot))) {
        prev = prev->next;
    }

    newNode->next = prev->next;
    prev->next = newNode;
}


void DeletePastMeetings(struct meetings** head) {
    time_t now = time(NULL);
    struct tm* current = localtime(&now);

    int currentHour = current->tm_hour;
    int currentDay = current->tm_mday;
    int currentMonth = current->tm_mon + 1;
    int currentYear = current->tm_year + 1900;

    while (*head != NULL) {
        struct meetSlot ms = (*head)->MS;

        bool isToday = (ms.day == currentDay && ms.month == currentMonth && ms.year == currentYear);
        bool isOld = (ms.year < currentYear) ||
                     (ms.year == currentYear && ms.month < currentMonth) ||
                     (ms.year == currentYear && ms.month == currentMonth && ms.day < currentDay);

        bool expiredToday = false;
        if (isToday) {
            if ((ms.slot == 'A' && currentHour >= 17) ||
                (ms.slot == 'B' && currentHour >= 18) ||
                (ms.slot == 'C' && currentHour >= 19)) {
                expiredToday = true;
            }
        }

        if (isOld || expiredToday) {
            struct meetings* temp = *head;
            *head = (*head)->next;
            free(temp);
        } else {
            break;
        }
    }
}

void LoadFromCSV(struct meetings** head, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        return;  // File doesn't exist yet - that's okay
    }

    char line[256];
    // Skip header if it exists
    if (fgets(line, sizeof(line), file) && 
        (strstr(line, "mentee") || strstr(line, "Mentee"))) {
        // It's a header, we can ignore it
    } else {
        rewind(file);  // No header, go back to start
    }

    while (fgets(line, sizeof(line), file)) {
        char mentee[100];
        struct meetSlot ms;
        int bookedValue;

        if (sscanf(line, "%99[^,],%c,%d,%d,%d,%d",
                  mentee, &ms.slot, &ms.day, &ms.month, &ms.year, &bookedValue) == 6) {
            ms.booked = (bookedValue == 1);
            Insert(head, mentee, ms);
        }
    }

    fclose(file);
}

void SaveAllToCSV(struct meetings* head, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        printf("Error: Could not open file for writing\n");
        return;
    }

    // Write header
    fprintf(file, "mentee,slot,day,month,year,booked\n");

    // Write all meetings
    struct meetings* temp = head;
    while (temp) {
        fprintf(file, "%s,%c,%d,%d,%d,%d\n",
               temp->mentee,
               temp->MS.slot,
               temp->MS.day,
               temp->MS.month,
               temp->MS.year,
               temp->MS.booked ? 1 : 0);
        temp = temp->next;
    }

    fclose(file);
}

void FreeMeetings(struct meetings* head) {
    while (head) {
        struct meetings* temp = head;
        head = head->next;
        free(temp);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 7) {
        printf("Error: Not enough arguments\n");
        printf("Usage: %s <name> <slot> <day> <month> <year> <booked>\n", argv[0]);
        return 1;
    }

    const char* mentee = argv[1];
    char slot = argv[2][0];
    int day = atoi(argv[3]);
    int month = atoi(argv[4]);
    int year = atoi(argv[5]);
    bool booked = atoi(argv[6]);

    struct meetSlot newSlot = {
        .slot = slot,
        .day = day,
        .month = month,
        .year = year,
        .booked = booked
    };

    struct meetings *front = NULL;
    
    // 1. Load existing meetings
    LoadFromCSV(&front, "meetings/meetings.csv");

    // 2. Add new meeting
    Insert(&front, mentee, newSlot);
    
    // 3. Clean up old meetings
    DeletePastMeetings(&front);
    
    // 4. Save all meetings
    SaveAllToCSV(front, "meetings/meetings.csv");

    printf("Success: Booking saved for %s on %d/%d/%d at slot %c\n",
          mentee, day, month, year, slot);
    
    FreeMeetings(front);

    return 0;
}
