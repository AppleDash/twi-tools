/* magic_bulk: takes in a list of image files on stdin, runs magic on them all.
 * writes successfully identified files to one file, unsuccessfuls to another. */
#include <magic.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define FLUSH 1 // whether or not to flush the files after each line written
#define STATS 1
#define MAX_FILE_LENGTH 1024 // seems reasonable, linux uses 4096 but that's like, a lot, dude.

magic_t magic; // magic_t is a pointer type

FILE *goodFp; // fp where "good" filenames are written
FILE *badFp; // fp where names of "bad" files are written

bool good_magic_info(const char *magicInfo) {
    if (!magicInfo) {
        return false;
    }

    // accept any image or video MIME type.
    return strstr(magicInfo, "image/") == magicInfo ||
            strstr(magicInfo, "video/") == magicInfo;
}

void process(const char *filename) {
    const char *magicInfo = magic_file(magic, filename);
    FILE *fp = good_magic_info(magicInfo) ? goodFp : badFp;

    if (!fprintf(fp, "%s\t%s\n", filename, magicInfo == NULL ? "null" : magicInfo)) {
        fprintf(stderr, "failed to write to file!");
        return;
    }

    
    #if FLUSH
    fflush(fp);
    #endif
}

int main(int argc, char *argv[]) {
    #if STATS
    int processed = 0;
    #endif

    int retval = 0;
    char buf[MAX_FILE_LENGTH + 1] = { 0 };

    if (argc != 3) {
        fprintf(stderr, "usage: %s <good file> <bad file>\n", argv[0]);
        return 1;
    }

    magic = magic_open(MAGIC_SYMLINK | MAGIC_MIME_TYPE); // follow symlinks because why not, we want MIME types.

    if (!magic || (magic_load(magic, NULL)) != 0) {
        fprintf(stderr, "failed to create magic handle\n");
        return 1;
    }

    goodFp = fopen(argv[1], "a");

    if (!goodFp) {
        fprintf(stderr, "failed to open \"good\" file %s\n", argv[1]);

        magic_close(magic);
        return 1;
    }

    badFp = fopen(argv[2], "a");

    if (!badFp) {
        fprintf(stderr, "failed to open \"bad\" file %s\n", argv[2]);

        fclose(goodFp);
        magic_close(magic);
        return 1;
    }

    while (fgets(buf, sizeof(buf), stdin) != 0) { // null on error or EOF
        int length = strlen(buf);
        bool good = false;

        while (length > 0 && (buf[length - 1] == '\r' || buf[length - 1] == '\n')) { // get rid of any newlines
            buf[length - 1] = 0;
            length--;
        }

        if (length > 0) {
            process(buf);
        }

        #if STATS
        processed++;

        if ((processed % 100) == 0) {
            printf("%d ", processed);
            fflush(stdout);
        }
        #endif
    }

    if (ferror(stdin)) {
        fprintf(stderr, "error reading from stdin\n");

        retval = 1;
    }

    #if STATS
    printf("total: %d\n", processed);
    #endif


    magic_close(magic);
    fclose(goodFp);
    fclose(badFp);

    return retval;
}