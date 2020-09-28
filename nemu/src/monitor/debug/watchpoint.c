#include "watchpoint.h"
#include "expr.h"
#include <stdlib.h>
#include <string.h>

#define NR_WP 32

static bool free_wp(WP *wp);

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

static WP* get_wp_pointer(int NO){
    for (int i = 0; i < NR_WP; ++i) {
        if (wp_pool[i].NO==NO) {
            // printf("Find %d 's ptr.\n",NO);
            return &wp_pool[i];
        }
    }
    return NULL;
}

static WP* linklist_insert(WP* linklist,WP* node){
    if (linklist==NULL){
        linklist=node;
        node->next=NULL;
    }
    else{
        node->next = linklist;
        linklist = node;
    }
    return linklist;
}

static WP* linklist_free(WP* linklist,WP* node){
    if (linklist==node){
        linklist=node->next;
        node->next=NULL;
        return linklist;
    }
    else{
        WP* this = linklist;
        while (this != NULL){
            if (this->next==node){
                this->next=node->next;
                node->next=NULL;
                return linklist;
            }
            else{
                this = this->next;
            }
        }
        return NULL;
    }
}

static bool init_wp(WP* wp,char* arg){
    strcpy(wp->arg,arg);
    bool success=false;
    wp->val = expr(arg,&success);
    return success;
}

static WP* new_wp(char* arg){
    if (free_==NULL) {
        printf("ERROR: watchpoint.c: No free watchpoint available.\n");
        return NULL;
    }
    WP* wp = free_;
    free_ = linklist_free(free_,wp);
    head = linklist_insert(head,wp);
    if (init_wp(wp,arg))
        return wp;
    else{
        printf("ERROR: watchpoint.c: Expression which is set to the watchpoint cannot be init.\n");
        free_wp(wp);
        return NULL;
    }
}

static bool free_wp(WP *wp) {
    WP* f = linklist_free(head,wp);
    if (f==NULL) {
        return false;
    }
    else{
        head = f;
        free_ = linklist_insert(free_,wp);
        return true;
    }
}

static void sort_linklist_with_id(WP* linklist){
    // todo
}

int set_watchpoint(char* args,bool* success){
    WP* wp = new_wp(args);
    if (wp==NULL){
        *success = false;
        return 0;
    }
    else{
        *success = true;
        return wp->NO;
    }
}

bool free_watchpoint(int NO){
    // printf("Try to free %d.\n",NO);
    WP* wp = get_wp_pointer(NO);
    if (wp==NULL) {
        printf("No such watchpoint.\n");
        return false;
    }
    return free_wp(wp);
}

bool update_all_watchpoint(){
    WP* ptr=head;
    bool flag = false;
    while (ptr!=NULL){
        bool success=false;
        word_t new_val = expr(ptr->arg,&success);
        if (success){
            if (new_val != ptr->val){
                printf("Watchpoint %d's value has been changed.\n",ptr->NO);
                ptr->val = new_val;
                flag = true;
            }
        }
        else{
            printf("Watchpoint %d failed to update its value.\n",ptr->NO);
        }
        ptr = ptr->next;
    }
    return flag;
}

void print_all_watchpoint(){
    update_all_watchpoint();
    printf("No. \tVal \t\t Args \n");
    if (head==NULL){
        printf("No watchpoint has been used.\n");
    }
    else{
        sort_linklist_with_id(head);
        WP* ptr = head;
        while (ptr!=NULL){
            printf("%d %12u \t\t %s\n",ptr->NO,ptr->val,ptr->arg);
            ptr = ptr->next;
        }
    }
}
