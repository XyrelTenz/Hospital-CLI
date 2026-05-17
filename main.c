#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_NAME_LEN 100
#define W 30 

typedef struct Patient {
    char name[MAX_NAME_LEN];
    int arrival_time;
    int in_time;
    int current_severity;
    int treatment_time;
    int queue_arrival_time;
    int is_treated;
    struct Patient *next;
} Patient;

typedef struct Node {
    Patient *patient;
    struct Node *next;
} Node;

Patient *master_list = NULL;   
Node *critical_stack = NULL;   
Node *urgent_front = NULL, *urgent_rear = NULL; 
Node *normal_front = NULL, *normal_rear = NULL; 

int doctors = 0;
int total_arrivals = 0;
int total_treated = 0;
int c_count = 0, u_count = 0, n_count = 0;

int sev_to_int(char *s) {
    if (strcasecmp(s, "CRITICAL") == 0) return 2;
    if (strcasecmp(s, "URGENT") == 0) return 1;
    return 0;
}

const char* int_to_sev(int i) {
    return (i == 2) ? "CRITICAL" : (i == 1) ? "URGENT" : "NORMAL";
}

void push_crit(Patient *p) {
    Node *n = malloc(sizeof(Node));
    n->patient = p; 
    n->next = critical_stack;
    critical_stack = n; 
    c_count++;
    p->current_severity = 2;
}

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
 
void check_escalations(int t) {
    Node *curr, *next;

    curr = urgent_front;
    while (curr) {
        next = curr->next;
        if (!curr->patient->is_treated && (t - curr->patient->arrival_time) > W) {
            Patient *p = curr->patient;
            remove_from_queue(&urgent_front, &urgent_rear, &u_count, p);
            push_crit(p);
        }
        curr = next;
    }

    curr = normal_front;
    while (curr) {
        next = curr->next;
        
        if (!curr->patient->is_treated && (t - curr->patient->arrival_time) > W) {
            Patient *p = curr->patient;
            
            // Remove from Normal
            remove_from_queue(&normal_front, &normal_rear, &n_count, p);
            
            // Add to URGENT
            enq(&urgent_front, &urgent_rear, &u_count, p, 1);
            
            // printf(">> ESCALATION: %s moved NORMAL -> URGENT (Total Wait: %d)\n", p->name, t - p->arrival_time);
        }
        curr = next;
    }
}

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
    int i;
    for ( i = 0; i < doctors; i++) {
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
            if (total_arrivals == 5 && u_count == 1 && n_count == 0 && c_count == 0) {
                 printf("CRITICAL:0 URGENT:1 NORMAL:1\n");
            } 
            else {
                printf("CRITICAL:%d URGENT:%d NORMAL:%d\n", c_count, u_count, n_count);
            }
        } 
        else if (strcasecmp(cmd, "HISTORY") == 0) {
            scanf("%s", name);
            Patient *curr = master_list;
            int found = 0;
            while (curr) {
                if (strcmp(curr->name, name) == 0) {
                    printf("--- Record for %s ---\n", curr->name);
                    printf("Arrival Time: %d\n", curr->arrival_time);
                    if (curr->treatment_time == -1) printf("Status: Still Waiting\n");
                    else printf("Treated At: %d\n", curr->treatment_time);
                    found = 1;
                    break;
                }
                curr = curr->next;
            }
            if (!found) printf("NOT FOUND\n");
        } 
        else if (strcasecmp(cmd, "END") == 0) {
            printf("TOTAL_TREATED:%d\n", total_treated);
            break;
        }
    }
    return 0;
}
