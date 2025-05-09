#ifndef MEETINGS_H
#define MEETINGS_H

#include <stdbool.h>

struct meetSlot {
    char slot;  
    int day;
    int month;
    int year;
    bool booked;
};


struct meetings {
    char mentee[100];
    struct meetSlot MS;
    struct meetings* next;
};

int isEmpty(struct meetings* front);
bool IsOverlapping(struct meetSlot newMS, struct meetSlot existingMS);
void Insert(struct meetings** head, const char* mentee, struct meetSlot ms);
void DeletePastMeetings(struct meetings** head);
void LoadFromCSV(struct meetings** head, const char* filename);
void SaveAllToCSV(struct meetings* head, const char* filename);
void FreeMeetings(struct meetings* head);

#endif
