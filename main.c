#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_NAME_LEN 100
#define W 30  // Waiting time limit (minutes/seconds) before an URGENT patient becomes CRITICAL


// The Patient structure acts as the "Medical Record" stored in our master list.
typedef struct Patient {
    char name[MAX_NAME_LEN];
    int arrival_time;
    int current_severity;   // 0 = NORMAL, 1 = URGENT, 2 = CRITICAL
    int treatment_time;     // Records when the doctor actually saw them
    int queue_arrival_time; // Used to track how long they've sat in their current tier
    int is_treated;         // Simple flag to check if they are still in the waiting room
    struct Patient *next;
} Patient;

// A standard Node used for our Stacks (Critical) and Queues (Urgent/Normal).
typedef struct Node {
    Patient *patient;
    struct Node *next;
} Node;

// Dito po makikita mga lahat ng record ng patient
Patient *master_list = NULL;   
// Itong Line of Code Po to check if its CRITICAL/NORMAL/URGENT Patients
Node *critical_stack = NULL;   
Node *urgent_front = NULL, *urgent_rear = NULL; 
Node *normal_front = NULL, *normal_rear = NULL; 

int doctors = 0;
int total_arrivals = 0;
int total_treated = 0;
int c_count = 0, u_count = 0, n_count = 0;


// This function convert nya yung input mo into numbers for example NORMAL
int sev_to_int(char *s) {
    if (strcasecmp(s, "CRITICAL") == 0) return 2;
    if (strcasecmp(s, "URGENT") == 0) return 1;
    return 0;
}

// Itong function naman e convert nya yung numbers into readable text for status reports.
const char* int_to_sev(int i) {
    return (i == 2) ? "CRITICAL" : (i == 1) ? "URGENT" : "NORMAL";
}


// Critical patients are handled like a stack—the most recent emergency gets immediate attention.
// If yung patient is CRITICAL it prioritize nya 
void push_crit(Patient *p) {
    Node *n = malloc(sizeof(Node));
    n->patient = p; 
    n->next = critical_stack;
    critical_stack = n; 
    c_count++;
    p->current_severity = 2;
}


// This function naman po think a receptionist sa hospital na nag hahandle sa mga bagong dating na patient but only Normal at Urgent if CRITICAL yung patient it bypass this function

// We use double pointer or ** dahil gusto natin ma change yung value permanently 
// ENQUEUE
void enq(Node **f, Node **r, int *cnt, Patient *p, int sev) {
    Node *n = malloc(sizeof(Node));
    n->patient = p; 
    n->next = NULL;
    if (!*f) *f = n; else (*r)->next = n;
    *r = n; 
    (*cnt)++;
    p->current_severity = sev;
    p->queue_arrival_time = p->arrival_time; 
}

// Same lang ito sa ENQUEUE function sa itaas pero ang role po nito is to DEQUEUE a patient meaning if other patient is tapos na sa pag assist proceed to another
Patient* deq(Node **f, Node **r, int *cnt) {
    if (!*f) return NULL;
    Node *t = *f; 
    Patient *p = t->patient;
    *f = (*f)->next; 
    if (!*f) *r = NULL;
    free(t); 
    (*cnt)--;
    return p;
}

// This helps us move a patient out of a lower-tier line when they get promoted or escalated.
// Itong yung function na e bypass yung QUEUES if yung patient is in CRITICAL meaning it automatically proceed to assits
// If dalawa naman yung critical tas my paparating na normal patient yung next na CRITICAL is still be assited after the other one is done
void remove_from_queue(Node **f, Node **r, int *cnt, Patient *p) {
    Node *curr = *f, *prev = NULL;
    while (curr) {
        if (curr->patient == p) {
            if (prev) prev->next = curr->next; else *f = curr->next;
            if (curr == *r) *r = prev;
            free(curr); 
            (*cnt)--; 
            return;
        }
        prev = curr; curr = curr->next;
    }
}


// This function naman is for URGENT patient na nag iintay ng more than sa waiting time which is 30 kung 30 up na siya nag iintay we transafer or escalate him/her to Critial or ER ROOm 
void check_escalations(int t) {
    Node *curr = urgent_front, *next;
    while (curr) {
        next = curr->next;
        if (!curr->patient->is_treated && (t - curr->patient->queue_arrival_time) > W) {
            Patient *p = curr->patient;
            remove_from_queue(&urgent_front, &urgent_rear, &u_count, p);
            push_crit(p);
        }
        curr = next;
    }
}

// Handles the logic for a new person walking into the ER.
void handle_arrival(char *name, char *sev_s, int t) {
    Patient *p = malloc(sizeof(Patient));
    strcpy(p->name, name); 
    p->arrival_time = t; 
    // -1 means "Not treated yet"
    p->treatment_time = -1; 
    p->is_treated = 0; 
    p->queue_arrival_time = t; 
    p->next = master_list;
    // Add to our master list of the new patient records
    master_list = p; 
    
    int s = sev_to_int(sev_s);
    if (s == 2) push_crit(p);
    else if (s == 1) enq(&urgent_front, &urgent_rear, &u_count, p, 1);
    else enq(&normal_front, &normal_rear, &n_count, p, 0);

    // Every 5 arrivals, the oldest Normal patient gets promoted to Urgent.
    total_arrivals++;
    if (total_arrivals % 5 == 0 && n_count > 0) {
        Patient *promoted = deq(&normal_front, &normal_rear, &n_count);
        enq(&urgent_front, &urgent_rear, &u_count, promoted, 1);
    }
    check_escalations(t);
}

// This function naman is to check kung sino uunahin e treat but e check muna ni DOCTOR yung status if its CRITICAL/URGENT/NORMAL
void handle_treat(int t) {
// Update priorities based on the current time
    check_escalations(t); 
    for (int i = 0; i < doctors; i++) {
        Patient *p = NULL;
        
        // Check the Critical stack
        if (critical_stack) {
            Node *temp = critical_stack; 
            p = temp->patient;
            critical_stack = critical_stack->next; 
            free(temp); 
            c_count--;
        } 
        // Check the Urgent queue
        else if (urgent_front) p = deq(&urgent_front, &urgent_rear, &u_count);
        // Check the Normal queue
        else if (normal_front) p = deq(&normal_front, &normal_rear, &n_count);

        if (p) {
            p->is_treated = 1; 
            p->treatment_time = t;
            printf("Treating %s %s %d\n", p->name, int_to_sev(p->current_severity), t);
            total_treated++;
        }
    }
}


int main() {
    char cmd[20], name[MAX_NAME_LEN], sev[20];
    int t;

    printf("ER System Ready. Commands: SET_DOCTORS, ARRIVE, TREAT, STATUS, HISTORY, END\n> ");

    while (scanf("%s", cmd) != EOF) {
        if (strcasecmp(cmd, "SET_DOCTORS") == 0) {
            scanf("%d", &doctors);
        } 
        else if (strcasecmp(cmd, "ARRIVE") == 0) {
            scanf("%s %s %d", name, sev, &t);
            handle_arrival(name, sev, t);
        } 
        else if (strcasecmp(cmd, "TREAT") == 0) {
            scanf("%d", &t);
            handle_treat(t);
        } 
        else if (strcasecmp(cmd, "STATUS") == 0) {
            // Quick snapshot of the current waiting room
            printf("CRITICAL:%d URGENT:%d NORMAL:%d\n", c_count, u_count, n_count);
        } 
        else if (strcasecmp(cmd, "HISTORY") == 0) {
            scanf("%s", name);
            Patient *curr = master_list;
            // Search the filing cabinet for the patient by name
            while (curr && strcmp(curr->name, name) != 0) curr = curr->next;
            
            if (!curr) {
                printf("NOT FOUND\n");
            } else {
                printf("--- Record for %s ---\n", curr->name);
                printf("Arrival Time: %d\n", curr->arrival_time);
                if (curr->treatment_time == -1) {
                    printf("Status: Still Waiting\n");
                } else {
                    printf("Treated At: %d\n", curr->treatment_time);
                }
            }
        } 
        else if (strcasecmp(cmd, "END") == 0) {
            printf("Final Report - Total Patients Treated: %d\n", total_treated);
            break;
        }

        printf("> ");
    }
    return 0;
}

// HOW TO USE THE SYSTEM?
/**
* 1. Set a DOCTOR for example SET_DOCTORS 1
*
* FORMAT is ARRIVE (PATIENT NAME) (PATIENT STATUS) (TIME THEY ARRIVE) 
* 2. Add new Arrival Patient for example ARRIVE xyrel NORMAL 0
* 3. TREAT the patient for example TREATING xyrel NORMAL 0
* 
* HOW TO CHECK THE HISTORY??
* HISTORY (PATIENT NAME)
* HISTORY xyrel
/
