#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#define MAX_MESSAGE_SIZE 1024

int main(int argc, char **argv)
{
   openlog("AESD", 0, LOG_USER);

   if (argc != 3)
   {
      printf("Failed: Number of parameter (%d) not valid\n", argc-1);
      syslog(LOG_ERR, "Failed: Number of parameter (%d) not valid\n", argc-1);
      exit(1);
   }

   // First argument is full path to a file (including filename) on the filesystem
   char* writefile = argv[1];
   // Second argument is a text string which will be written within this file
   char* writestr = argv[2];
   unsigned int writeStrLen = strlen(writestr);

   printf("Writing %s to %s\n", writestr, writefile);
   syslog(LOG_DEBUG, "Writing %s to %s\n", writestr, writefile);
   FILE *fp=fopen(writefile,"wb");
   if (fp == NULL)
   {
      printf("Failed: Unable to open file.\n");
      syslog(LOG_ERR, "Failed: Unable to open file.\n");
      exit(1);
   }

   unsigned int numWritten = fwrite(writestr, 1, writeStrLen, fp);
   if (numWritten != writeStrLen)
   {
      printf("Failed: Unable to write %d bytes. Wrote %d bytes.\n", writeStrLen, numWritten);
      syslog(LOG_ERR, "Failed: Unable to write %d bytes. Wrote %d bytes.\n", writeStrLen, numWritten);
      exit(1);
   }
   printf("Wrote string.\n");

   fflush(fp);
   fclose(fp);

   return 0;
}

#if 0

set -e
set -u

if [ $# -lt 2 ]
then
	echo "Failed: Parameter(s) not specified"
	exit 1
fi	

# First argument is full path to a file (including filename) on the filesystem
Writefile=$1
# Second argument is a text string which will be written within this file
Writestr=$2

mkdir -p  "$(dirname ${Writefile})" #newFolder2/test.txt
if [ $? -ne 0 ]; then
   echo "Failed to create."
   exit 1
fi

echo "${Writestr}" > "${Writefile}"
if [ $? -ne 0 ]; then
   echo "Failed to write."
   exit 1
fi

#endif
