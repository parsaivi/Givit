#include "tag.h"
#include "utils.h"
#include "commit.h"
#include "repository.h"

static void sort_tags(void)
{
    FILE *tags = fopen(".givit/tags", "r");
    if (tags == NULL) return;

    char lines[1024][MAX_LINE_LEN];
    char names[1024][MAX_NAME_LEN];
    int count = 0;
    char temp[MAX_LINE_LEN];

    while (fgets(temp, MAX_LINE_LEN, tags) != NULL && count < 1024) {
        strip_newline(temp);
        strcpy(lines[count], temp);
        sscanf(temp, "\"%[^\"]\"", names[count]);
        count++;
    }
    fclose(tags);

    /* bubble sort by tag name */
    for (int i = 0; i < count; i++) {
        for (int j = i + 1; j < count; j++) {
            if (strcmp(names[i], names[j]) > 0) {
                char tmp_line[MAX_LINE_LEN];
                char tmp_name[MAX_NAME_LEN];
                strcpy(tmp_line, lines[i]);
                strcpy(lines[i], lines[j]);
                strcpy(lines[j], tmp_line);
                strcpy(tmp_name, names[i]);
                strcpy(names[i], names[j]);
                strcpy(names[j], tmp_name);
            }
        }
    }

    FILE *out = fopen(".givit/tags", "w");
    for (int i = 0; i < count; i++) {
        fprintf(out, "%s\n", lines[i]);
    }
    fclose(out);
}

static bool tag_exists(const char *tag_name)
{
    FILE *tags = fopen(".givit/tags", "r");
    if (tags == NULL) return false;

    char temp[MAX_LINE_LEN];
    while (fgets(temp, MAX_LINE_LEN, tags) != NULL) {
        strip_newline(temp);
        char name[MAX_NAME_LEN];
        sscanf(temp, "\"%[^\"]\"", name);
        if (strcmp(name, tag_name) == 0) {
            fclose(tags);
            return true;
        }
    }
    fclose(tags);
    return false;
}

static void create_tag(const char *tag_name, int commit_id, const char *message)
{
    char username[MAX_NAME_LEN] = "", email[MAX_NAME_LEN] = "";
    char branch[MAX_NAME_LEN] = "", parent_branch[MAX_NAME_LEN] = "";
    repo_read_config(username, email, branch, parent_branch);

    time_t now = time(NULL);

    FILE *tags = fopen(".givit/tags", "a");
    fprintf(tags, "\"%s\" %d \"%s\" \"%s\" %ld \"%s\"\n",
            tag_name, commit_id, username, email, (long)now, message);
    fclose(tags);

    sort_tags();
}

int run_tag(int argc, char *argv[])
{
    Commit *head = commit_load_list(".givit/commitsdb");

    if (argc == 2) {
        /* list all tags */
        FILE *tags = fopen(".givit/tags", "r");
        if (tags == NULL) {
            commit_free_list(head);
            return 0;
        }
        char temp[MAX_LINE_LEN];
        while (fgets(temp, MAX_LINE_LEN, tags) != NULL) {
            strip_newline(temp);
            char name[MAX_NAME_LEN];
            sscanf(temp, "\"%[^\"]\"", name);
            printf("%s\n", name);
        }
        fclose(tags);
        commit_free_list(head);
        return 0;
    }

    if (strcmp(argv[2], "show") == 0) {
        if (argc < 4) {
            perror("please enter a valid command");
            commit_free_list(head);
            return 1;
        }
        FILE *tags = fopen(".givit/tags", "r");
        if (tags == NULL) {
            perror("tags file not found");
            commit_free_list(head);
            return 1;
        }
        char temp[MAX_LINE_LEN];
        while (fgets(temp, MAX_LINE_LEN, tags) != NULL) {
            strip_newline(temp);
            char tag_name[MAX_NAME_LEN], author[MAX_NAME_LEN], email[MAX_NAME_LEN], msg[MAX_MSG_LEN];
            int cid;
            long ts;
            sscanf(temp, "\"%[^\"]\" %d \"%[^\"]\" \"%[^\"]\" %ld \"%[^\"]\"",
                   tag_name, &cid, author, email, &ts, msg);
            if (strcmp(argv[3], tag_name) == 0) {
                time_t timestamp = (time_t)ts;
                struct tm *tm_info = localtime(&timestamp);
                printf("\e[37mGivit tag show %s:\e[m\n", tag_name);
                printf("\ttag %s\n", tag_name);
                printf("\tcommit %d\n", cid);
                printf("\tAuthor: %s %s\n", author, email);
                printf("\tDate: %d:%d:%d\n", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
                printf("\tMessage: %s\n", msg);
                fclose(tags);
                commit_free_list(head);
                return 0;
            }
        }
        printf("tag '%s' not found\n", argv[3]);
        fclose(tags);
        commit_free_list(head);
        return 1;
    }

    if (strcmp(argv[2], "-a") == 0) {
        if (argc < 4) {
            perror("please enter a valid command");
            commit_free_list(head);
            return 1;
        }

        char tag_name[MAX_NAME_LEN];
        strncpy(tag_name, argv[3], MAX_NAME_LEN - 1);
        tag_name[MAX_NAME_LEN - 1] = '\0';

        char message[MAX_MSG_LEN] = "";
        int commit_id = head ? head->id : 0;
        bool force = false;

        for (int i = 4; i < argc; i++) {
            if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
                strncpy(message, argv[i + 1], MAX_MSG_LEN - 1);
                message[MAX_MSG_LEN - 1] = '\0';
                i++;
            } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
                commit_id = atoi(argv[i + 1]);
                i++;
            } else if (strcmp(argv[i], "-f") == 0) {
                force = true;
            }
        }

        if (!force && tag_exists(tag_name)) {
            perror("tag already exists");
            commit_free_list(head);
            return 1;
        }

        create_tag(tag_name, commit_id, message);
        commit_free_list(head);
        return 0;
    }

    perror("please enter a valid command");
    commit_free_list(head);
    return 1;
}
