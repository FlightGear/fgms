/*
 * testdb.c - test a connection to the prostgresql database
 *
 *   Author: Geoff R. McLane <reports@geoffair.info>
 *   License: GPL
 *
 *   Revision 0.1  2012/07/02 geoff
 *   Initial cut. Some testing routines, searching alternatives
 *   Some code cut from : http://www.postgresql.org/docs/8.0/static/libpq-example.html
 *
 */

#include "common.h"
#include "wrappers.h"
#include "error.h"
#include <libpq-fe.h>

/* DEFAULT DATABASE INFORMATION IF NOT GIVEN ON THE COMMAND LINE */

#ifndef DEF_IP_ADDRESS
#define DEF_IP_ADDRESS "192.168.1.105"
#endif

#ifndef DEF_PORT
#define DEF_PORT "5432"
#endif

#ifndef DEF_DATABASE
#define DEF_DATABASE "fgtracker"
#endif

#ifndef DEF_USER_LOGIN
#define DEF_USER_LOGIN "fgtracker"
#endif

#ifndef DEF_USER_PWD
#define DEF_USER_PWD "fgtracker"
#endif

static char *ip_address = (char *)DEF_IP_ADDRESS;
static char *port = (char *)DEF_PORT;
static char *database = (char *)DEF_DATABASE;
static char *user = (char *)DEF_USER_LOGIN;
static char *pwd = (char *)DEF_USER_PWD;
static char *pgoptions = (char *)"";
static char *pgtty = (char *)"";

static int got_flights = 0;
static int got_waypts = 0;

#define PQ_EXEC_SUCCESS(res) ((PQresultStatus(res) == PGRES_COMMAND_OK)||(PQresultStatus(res) == PGRES_TUPLES_OK))


/* --------------------------------------
PGconn *PQsetdbLogin(const char *pghost,
                     const char *pgport,
                     const char *pgoptions,
                     const char *pgtty,
                     const char *dbName,
                     const char *login,
                     const char *pwd);
   ------------------------------------- */

static void exit_nicely(PGconn* conn)
{
	PQfinish(conn);
   exit(1);
}

void test_template1(void)
{
     char *pghost, *pgport, *pgoptions, *pgtty;
     char* dbName;
     int nFields;
     int i,j;

   /*  FILE *debug; */

     PGconn* conn;
     PGresult* res;

     /* begin, by setting the parameters for a backend connection
        if the parameters are null, then the system will try to use
        reasonable defaults by looking up environment variables
        or, failing that, using hardwired constants */
     pghost = NULL;  /* host name of the backend server */
     pgport = NULL;  /* port of the backend server */
     pgoptions = NULL; /* special options to start up the backend server */
     pgtty = NULL;     /* debugging tty for the backend server */
     dbName = "template1";

     /* make a connection to the database */
     conn = PQsetdb(pghost, pgport, pgoptions, pgtty, dbName);

     /* check to see that the backend connection was successfully made */
     if (PQstatus(conn) == CONNECTION_BAD) {
       fprintf(stderr,"Connection to database '%s' failed. ", dbName);
       fprintf(stderr,"[%s]\n",PQerrorMessage(conn));
       exit_nicely(conn);
     }

   /*  debug = fopen("/tmp/trace.out","w");  */
   /*   PQtrace(conn, debug);  */

     /* start a transaction block */

     res = PQexec(conn,"BEGIN");
     if (PQresultStatus(res) != PGRES_COMMAND_OK) {
       fprintf(stderr,"BEGIN command failed!\n");
       PQclear(res);
       exit_nicely(conn);
     }
     /* should PQclear PGresult whenever it is no longer needed to avoid
        memory leaks */
     PQclear(res);

     /* fetch instances from the pg_database, the system catalog of databases*/
     res = PQexec(conn,"DECLARE myportal CURSOR FOR select * from pg_database");
     if (PQresultStatus(res) != PGRES_COMMAND_OK) {
       fprintf(stderr,"DECLARE CURSOR command failed!\n");
       PQclear(res);
       exit_nicely(conn);
     }
     PQclear(res);

     res = PQexec(conn,"FETCH ALL in myportal");
     if (PQresultStatus(res) != PGRES_TUPLES_OK) {
       fprintf(stderr,"FETCH ALL command didn't return tuples properly\n");
       PQclear(res);
       exit_nicely(conn);
     }

     /* first, print out the attribute names */
     nFields = PQnfields(res);
     for (i=0; i < nFields; i++) {
       printf("%-15s",PQfname(res,i));
     }
     printf("\n");

     /* next, print out the instances */
     for (i=0; i < PQntuples(res); i++) {
       for (j=0  ; j < nFields; j++) {
         printf("%-15s", PQgetvalue(res,i,j));
       }
       printf("\n");
     }

     PQclear(res);

     /* close the portal */
     res = PQexec(conn, "CLOSE myportal");
     PQclear(res);

     /* end the transaction */
     res = PQexec(conn, "END");
     PQclear(res);

     /* close the connection to the database and cleanup */
     PQfinish(conn);

   /*   fclose(debug); */
}


void query_tables(PGconn *conn, int verb)
{
	PGresult		*res;
	char buff[MAXLINE];
	char *cp = buff;
	char *val;
	int i, j, i2, nFields, nRows;
	
   if (PQstatus(conn) == CONNECTION_OK) {
		res = PQexec(conn,"BEGIN");
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      	    fprintf(stderr,"BEGIN command failed!\n");
            PQclear(res);
            return;
        }
        /* should PQclear PGresult whenever it is no longer needed to avoid memory leaks */
        PQclear(res);
        strcpy(cp,"SELECT table_name FROM information_schema.tables WHERE table_schema = 'public';");
		res = PQexec(conn, cp);
	    if (PQ_EXEC_SUCCESS(res)) {
	       	nFields = PQnfields(res);
	       	nRows = PQntuples(res);
	       	printf("PQexec(%s) succeeded... %d fields, %d rows\n",cp,nFields,nRows);
	       	if (verb) {
	           	if (nFields > 1) {
	           	    printf("List of %d fields...\n",nFields);
	               	for (i = 0; i < nFields; i++) {
	               		printf("%s ", PQfname(res, i));
	               	}
               		printf("\n");
                }
                /* next, print out the rows */
                for (j = 0; j < nFields; j++) {
               		printf("Field: [%s]\n", PQfname(res, j));
                    for (i = 0; i < nRows; i++) {
                        i2 = i + 1;
                        val = PQgetvalue(res, i, j);
                        if (val) {
                            printf(" Row %3d: [%s]\n", i2, val);
                            if (strcmp(val,"flights") == 0)
                                got_flights = 1;
                            else if (strcmp(val,"waypoints") == 0)
                                got_waypts = 1;
                                
                        } else
                            printf(" Row %3d: [%s]\n", i2, "<null>");
                    }
                }	   
            }
	   } else {
    	   	printf("PQexec(%s) FAILED: [%s]\n",cp, PQerrorMessage(conn));
       }
	   PQclear(res);
       /* end the transaction */
       res = PQexec(conn, "END");
       PQclear(res);		
   } else {
		printf("ERROR: connection to database not valid: [%s]\n", PQerrorMessage(conn) );
   }
}


int test_alternate_connection(void)
{
    int iret = 0; // assume success
    PGconn *conn = NULL;
	char buff[MAXLINE];
	char *cp = buff;
	sprintf(cp,"hostaddr=%s port=%s dbname=%s user=%s password=%s",
		ip_address, port, database, user, pwd );
	conn = PQconnectdb(cp);
    if (PQstatus(conn) == CONNECTION_OK) {
		printf("Alternate connection [%s] was also successful.\n",cp);
		query_tables(conn,0); // again show the tables, but not all repeated info
	} else {
		printf("ERROR: connection not valid: %s\n", PQerrorMessage(conn) );
		iret = 1;
   }		
   PQfinish(conn);
   return iret;
}

int ConnectDB(PGconn **conn)
{
    int iret = 0;   /* assume no error */
    // char debugstr[MAXLINE];

    *conn = PQsetdbLogin(ip_address, port, pgoptions, pgtty, database, user, pwd);

    if (PQstatus(*conn) != CONNECTION_OK)
    {
        printf("ERROR: Connection to database failed: %s\n", PQerrorMessage(*conn) );
        //debug(1,debugstr);
        PQfinish(*conn);
        iret = 1;
    }
    return iret;
}

char *get_base_name(char *name)
{
    char *bn = strdup(name);
    size_t len = strlen(bn);
    size_t i, off;
    int c;
    off = 0;
    for (i = 0; i < len; i++) {
        c = bn[i];
        if (( c == '/' )||( c == '\\' ))
            off = i + 1;
    }
    return &bn[off];
}

void give_help(char *name)
{
    char *bn = get_base_name(name);
    printf("%s - version 0.1, compiled %s, at %s\n", bn, __DATE__, __TIME__);
    printf(" --help    (-h. -?) = This help, and exit(0).\n");
    printf(" --version          = This help, and exit(0).\n");
    printf(" --db database (-d) = Set the database name. (def=%s)\n",database);
    printf(" --ip addr     (-i) = Set the IP address of the postgresql server. (def=%s)\n",ip_address);
    printf(" --port val    (-p) = Set the port of the postgreql server. (def=%s)\n",port);
    printf(" --user name   (-u) = Set the user name. (def=%s)\n",user);
    printf(" --word pwd    (-w) = Set the password for the above user. (def=%s)\n",pwd);
}

int parse_commands( int argc, char **argv )
{
    int i, i2;
    char *arg;
    char *sarg;
    
    for ( i = 1; i < argc; i++ ) {
        i2 = i + 1;
        arg = argv[i];
        sarg = arg;
        if ((strcmp(arg,"--help") == 0) || (strcmp(arg,"-h") == 0) ||
            (strcmp(arg,"-?") == 0) || (strcmp(arg,"--version") == 0)) {
            give_help(argv[0]);
            exit(0);
        } else if (*sarg == '-') {
            sarg++;
            while (*sarg == '-') sarg++;
            switch (*sarg) 
            {
            case 'd':
                if (i2 < argc) {
                    sarg = argv[i2];
                    database = strdup(sarg);
                    i++;
                } else {
                    printf("database name must follow!\n");
                    goto Bad_ARG;
                }
                break;
            case 'i':
                if (i2 < argc) {
                    sarg = argv[i2];
                    ip_address = strdup(sarg);
                    i++;
                } else {
                    printf("IP address must follow!\n");
                    goto Bad_ARG;
                }
                break;
            case 'p':
                if (i2 < argc) {
                    sarg = argv[i2];
                    port = strdup(sarg);
                    i++;
                } else {
                    printf("port value must follow!\n");
                    goto Bad_ARG;
                }
                break;
            case 'u':
                if (i2 < argc) {
                    sarg = argv[i2];
                    user = strdup(sarg);
                    i++;
                } else {
                    printf("user name must follow!\n");
                    goto Bad_ARG;
                }
                break;
            case 'w':
                if (i2 < argc) {
                    sarg = argv[i2];
                    pwd = strdup(sarg);
                    i++;
                } else {
                    printf("password must follow!\n");
                    goto Bad_ARG;
                }
                break;
            default:
                goto Bad_ARG;
            }
        } else {
Bad_ARG:
            printf("ERROR: Unknown argument [%s]! Try -?\n",arg);
            return 1;
        }
    }
    return 0;
}



int main( int argc, char **argv )
{
    int res;
    PGconn *conn = NULL;
    
    if (parse_commands(argc,argv))
        return 1;

    // test_template1(); // this will fail unless there is a $USER users with $HOME/.passwd being the password
    
    printf("Attempting connection on [%s], port [%s], database [%s], user [%s], pwd [%s]\n",
        ip_address, port, database, user, pwd );

    res = ConnectDB(&conn);
    if (res)
        return 1;

    printf("Connection successful...\n");
    // sleep(1);
    
    printf("Query tables...\n");
	query_tables(conn,1);   // query and show fields, rows

    printf("Closing connection...\n");
    PQfinish(conn);
    
    res = test_alternate_connection();

    if (got_flights && got_waypts) {
        printf("database %s has 'flights' and 'waypoints' tables\n so appears suitable for 'fgt_server' use. Congrats!\n",database);
    }
    // printf("End testdb - return(0)\n");
    return 0;
}

/* eof - testdb.c */

