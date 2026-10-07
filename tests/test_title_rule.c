#include "title_rule.h"
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char directory[256], list[300];

static void write_list(const char *text, mode_t mode)
{
    unlink(list);
    int fd = open(list, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    assert(fd >= 0);
    assert(write(fd, text, strlen(text)) == (ssize_t)strlen(text));
    close(fd);
    assert(!chmod(list, mode));
}

static enum lapy_title_rule_result check(const char *title)
{
    return lapy_title_rule_check(title, "FAKE78", list, getuid());
}

int main(void)
{
    char link_path[320], hard_path[320];

    /* range */
    assert(lapy_title_in_range("FAKE78090", "FAKE78"));
    assert(lapy_title_in_range("FAKE78999", "FAKE78"));
    assert(!lapy_title_in_range("FAKE77031", "FAKE78"));
    assert(!lapy_title_in_range("PPSA78090", "FAKE78"));
    assert(!lapy_title_in_range("FAKE7809", "FAKE78"));
    assert(!lapy_title_in_range("FAKE780900", "FAKE78"));
    assert(!lapy_title_in_range("FAKE78A90", "FAKE78"));
    assert(lapy_title_in_range("FAKE78090", "FAKE"));
    assert(lapy_title_in_range("FAKE78090", "FAKE7"));
    assert(!lapy_title_in_range("FAKE78090", "fake78"));    /* bad prefix */
    assert(!lapy_title_in_range("FAKE78090", "FAKE78090")); /* a prefix must leave a digit */

    snprintf(directory, sizeof(directory), "/tmp/lapy-title-rule-%d", (int)getpid());
    snprintf(list, sizeof(list), "%s/allow", directory);
    assert(!mkdir(directory, 0755));

    assert(check("FAKE78090") == LAPY_TITLE_RULE_LIST_MISSING);
    write_list("FAKE78090\nFAKE78091\n", 0644);
    assert(check("FAKE78090") == LAPY_TITLE_RULE_ALLOWED);
    assert(check("FAKE78091") == LAPY_TITLE_RULE_ALLOWED);
    assert(check("FAKE78092") == LAPY_TITLE_RULE_NOT_LISTED);
    assert(check("FAKE77031") == LAPY_TITLE_RULE_OUT_OF_RANGE);
    assert(check("PPSA99995") == LAPY_TITLE_RULE_OUT_OF_RANGE);
    assert(lapy_title_rule_check("FAKE78090", "FAKE78", list, getuid() + 1) ==
           LAPY_TITLE_RULE_LIST_UNSAFE);               /* owned by someone else */

    /* partial lines and near-matches never count */
    write_list("FAKE78090", 0644);
    assert(check("FAKE78090") == LAPY_TITLE_RULE_NOT_LISTED);
    write_list("FAKE780901\n FAKE78090\nFAKE78090 \n", 0644);
    assert(check("FAKE78090") == LAPY_TITLE_RULE_NOT_LISTED);

    /* writable by others, by the group, or the directory is */
    write_list("FAKE78090\n", 0666);
    assert(check("FAKE78090") == LAPY_TITLE_RULE_LIST_UNSAFE);
    write_list("FAKE78090\n", 0664);
    assert(check("FAKE78090") == LAPY_TITLE_RULE_LIST_UNSAFE);
    write_list("FAKE78090\n", 0644);
    assert(!chmod(directory, 0777));
    assert(check("FAKE78090") == LAPY_TITLE_RULE_LIST_UNSAFE);
    assert(!chmod(directory, 0755));
    assert(check("FAKE78090") == LAPY_TITLE_RULE_ALLOWED);

    /* a second link to the list, or the list as a symbolic link */
    snprintf(hard_path, sizeof(hard_path), "%s/hard", directory);
    assert(!link(list, hard_path));
    assert(check("FAKE78090") == LAPY_TITLE_RULE_LIST_UNSAFE);
    unlink(hard_path);
    snprintf(link_path, sizeof(link_path), "%s/real", directory);
    assert(!rename(list, link_path));
    assert(!symlink(link_path, list));
    assert(check("FAKE78090") == LAPY_TITLE_RULE_LIST_UNSAFE);
    unlink(list);
    unlink(link_path);

    /* too big */
    {
        char *big = malloc(LAPY_TITLE_ALLOW_LIST_MAX + 11);
        assert(big);
        memset(big, '\n', LAPY_TITLE_ALLOW_LIST_MAX + 10);
        memcpy(big, "FAKE78090\n", 10);
        big[LAPY_TITLE_ALLOW_LIST_MAX + 10] = 0;
        write_list(big, 0644);
        free(big);
        assert(check("FAKE78090") == LAPY_TITLE_RULE_LIST_UNSAFE);
    }
    unlink(list);
    assert(!rmdir(directory));
    assert(lapy_title_rule_check("FAKE78090", "FAKE78", "relative", getuid()) ==
           LAPY_TITLE_RULE_LIST_UNSAFE);
    puts("title_rule tests passed");
    return 0;
}
