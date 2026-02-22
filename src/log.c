#define _XOPEN_SOURCE 700
#include "commit.h"
#include "utils.h"

#define SEPARATOR "\e[37m><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<\e[m"

static void print_commit(Commit *node)
{
    char timebuf[64];
    struct tm *tm_info = localtime(&node->timestamp);
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", tm_info);

    printf("commit id: %d\n", node->id);
    printf("message: %s\n", node->message);
    printf("user: %s\n", node->author_name);
    printf("email: %s\n", node->author_email);
    printf("branch: %s\n", node->branch);
    printf("parent branch: %s\n", node->parent_branch);
    printf("time: %s\n", timebuf);
    printf("snapshot: %s\n", node->snapshot_path);
    printf("%s\n", SEPARATOR);
}

static time_t parse_date(const char *datestr)
{
    struct tm tm_val;
    memset(&tm_val, 0, sizeof(tm_val));

    if (strptime(datestr, "%Y-%m-%d", &tm_val) != NULL) {
        return mktime(&tm_val);
    }
    if (strptime(datestr, "%H:%M:%S", &tm_val) != NULL) {
        return mktime(&tm_val);
    }
    return (time_t)-1;
}

int run_log(int argc, char *argv[])
{
    Commit *head = commit_load_list(".givit/commitsdb");
    if (head == NULL) {
        perror("no commit has been made yet");
        return 1;
    }

    printf("%s\n", SEPARATOR);

    Commit *node = head;

    if (argc > 2) {
        if (strcmp(argv[2], "-n") == 0) {
            int n = atoi(argv[3]);
            int count = 0;
            while (node != NULL && count < n) {
                print_commit(node);
                node = node->prev;
                count++;
            }
            commit_free_list(head);
            return 0;
        }

        if (strcmp(argv[2], "-branch") == 0) {
            while (node != NULL) {
                if (strcmp(node->branch, argv[3]) == 0)
                    print_commit(node);
                node = node->prev;
            }
            commit_free_list(head);
            return 0;
        }

        if (strcmp(argv[2], "-author") == 0) {
            while (node != NULL) {
                if (strcmp(node->author_name, argv[3]) == 0)
                    print_commit(node);
                node = node->prev;
            }
            commit_free_list(head);
            return 0;
        }

        if (strcmp(argv[2], "-since") == 0) {
            time_t since = parse_date(argv[3]);
            while (node != NULL) {
                if (node->timestamp > since)
                    print_commit(node);
                node = node->prev;
            }
            commit_free_list(head);
            return 0;
        }

        if (strcmp(argv[2], "-before") == 0) {
            time_t before = parse_date(argv[3]);
            while (node != NULL) {
                if (node->timestamp < before)
                    print_commit(node);
                node = node->prev;
            }
            commit_free_list(head);
            return 0;
        }

        if (strcmp(argv[2], "-search") == 0) {
            while (node != NULL) {
                for (int i = 3; i < argc; i++) {
                    if (strstr(node->message, argv[i]) != NULL) {
                        print_commit(node);
                        break;
                    }
                }
                node = node->prev;
            }
            commit_free_list(head);
            return 0;
        }
    }

    while (node != NULL) {
        print_commit(node);
        node = node->prev;
    }

    commit_free_list(head);
    return 0;
}
