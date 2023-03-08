/* Copyright (c) 2005-23, Alexander Holupirek <alex@holupirek.de>, BSD license */
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <time.h>

#include "basexdbc.h"
/* once libbasexdbc.so is installed in /usr/include/basex/ use:
#include "basex/basexdbc.h"
*/

#define DBHOST   "localhost"
#define DBPORT   "1984"
#define DBUSER   "admin"
#define DBPASSWD "admin"

#define FILE_OK 0
#define FILE_NOT_EXIST 1
#define FILE_TOO_LARGE 2
#define FILE_READ_ERROR 3

char * c_read_file(const char * f_name, int * err, size_t * f_size) {
    char * buffer;
    size_t length;
    size_t total_length;
    FILE * f = fopen(f_name, "rb");
    size_t read_length;

    if (f) {
        const char *xquery = "<create-db name='TEST_1'>";
        const char *xqueryEnd = " </create-db>";

        fseek(f, 0, SEEK_END);
        length = ftell(f);
        total_length = length + strlen(xquery) + strlen(xqueryEnd);
        fseek(f, 0, SEEK_SET);

        if (length > 3826003525) {
            *err = FILE_TOO_LARGE;

            return NULL;
        }

        buffer = (char *)malloc(total_length);


        strcpy (buffer, xquery);


        char * itbegin = buffer + sizeof(char)*strlen(xquery);

        if (length) {
            read_length = fread(itbegin, 1, length, f);

            if (length != read_length) {
                free(buffer);
                *err = FILE_READ_ERROR;

                return NULL;
            }
        }

        fclose(f);

        char * itEnd = buffer + length + strlen(xquery);

        strcpy (itEnd, xqueryEnd);

        length += strlen(xqueryEnd)+1;

        *err = FILE_OK;
        buffer[total_length] = '\0';
        *f_size = length;
    }
    else {
        *err = FILE_NOT_EXIST;

        return NULL;
    }

    return buffer;
}
/*
 * Example to demonstrate communication with running BaseX database server.
 *
 * $ cc -L. -lbasexdbc example.c -o example
 */
int
main(void)
{
	int sfd, rc, rc1;

	/* Connect to server and receive socket descriptor for this session. */
	sfd = basex_connect(DBHOST, DBPORT);
	if (sfd == -1) {
		warnx("Can not connect to BaseX server.");
		return 0;
	}

	/* We are connected, let's authenticate for this session. */
	rc = basex_authenticate(sfd, DBUSER, DBPASSWD);
	if (rc == -1) {
		warnx("Access to DB denied.");
		goto out;
	}


	int err;
	size_t f_size;
	char * f_data;

	f_data = c_read_file("3_8_GB.xml", &err, &f_size);

	/* Send command in default mode and receive the result string. */
	const char *command = f_data;//"xquery 1 + 1";
	char *result;
	char *info;

	char *result1;
	char *info1;

	if(!f_data)
	{
		warnx("Read File Failed.");
		goto free_and_out;
	}

	warnx("Read File Successfully.");

	const char *commandX = "SET ADDCACHE true";

	rc1 = basex_execute(sfd, commandX, &result1, &info1);
	if (rc1 == -1) { // general (i/o or the like) error
		warnx("SET ADDCACHE true: An error occurred during execution of '%s'.", command);
		goto free_and_out;
	}
	if (rc1 == 1) { // database error while processing command
		warnx("SET ADDCACHE true: Processing of '%s' failed.", command);
	}

	/* print command, result and info/error */
	printf("command: '%s'\n", commandX);
	printf("result : '%s'\n", result1);
	printf("%s : '%s'\n", (rc == 1) ? "error" : "info", info1);


	clock_t start = clock();

	rc = basex_execute(sfd, command, &result, &info);
	if (rc == -1) { // general (i/o or the like) error
		warnx("An error occurred during execution of '%s'.", command);
		goto free_and_out;
	}
	if (rc == 1) { // database error while processing command
		warnx("Processing of '%s' failed.", command);
	}
	clock_t stop = clock();
	double elapsed = (double)(stop - start) * 1000.0 / CLOCKS_PER_SEC;
    printf("\n\n\nTime elapsed in ms: %f \n\n\n", elapsed);

	/* print command, result and info/error */
	printf("command: '%s'\n", command);
	printf("result : '%s'\n", result);
	printf("%s : '%s'\n", (rc == 1) ? "error" : "info", info);

free_and_out:
	free(result);
	free(info);
out:
	basex_close(sfd);
	return 0;
}
