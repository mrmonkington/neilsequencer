// dir2c by MAKK
// coded for the M4 project
// scans the current dir for .h files and generates waves.cpp
// and wavename.inc from their names
// compile with djgpp

#include <dos.h>
#include <stdio.h>

main()
{
    struct find_t f;
    int i, j, k;
    FILE *op, *op2;
    char nameTab[500][16];
    char temp[500*16];
    char *t;

    op = fopen( "waves.cpp", "w");
    op2 = fopen( "wavename.inc", "w");

    fprintf( op, "short waves[] = {\n");
    fprintf( op2, "static char *wavenames[] = {\n");

    _dos_findfirst( "*.h", 0, &f);
    strcpy( nameTab[0], f.name);
    i = 1;
    while( !_dos_findnext( &f)) {
      strcpy( nameTab[i], f.name);
      for( j=0; j<i; j++)
           if( strcmp( f.name, nameTab[j]) < 0) {
                memcpy( temp, nameTab[j], (500-j)*16);
                strcpy( nameTab[j], f.name);
                memcpy( nameTab[j+1], temp, (500-j-1)*16);
                break;
           }

      i++;
    }

    for( j=0; j<i; j++) {
         fprintf( op, "#include \"%s\"\n", nameTab[j]);
         t = (char*)strchr( nameTab[j], '.');
         *t = 0;
         fprintf( op2, "\"%s\",\n", strlwr(nameTab[j]));
    }

    fprintf( op, "0 };");
    fprintf( op2, "\"noise\" };");
    printf( "\n%i files\n", i);
    fclose( op);
    fclose( op2);
}
