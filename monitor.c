#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/stat.h>
/* Project Include Files */
#include "utils.h"
#include "linked-list.h"
#include "lattice.h"
#include "monitor.h"

/* Globals */
list system_mapping;
int mic = 0;
int cmdCt = 1;



/**********************************************************************

    Function    : labelFile
    Description : Compute label (level) for file given process creator (optional)
    Inputs      : fh - output file pointer
                  proc - process name
                  file - file name
    Outputs     : 0 if successfully completed, -1 if failure

***********************************************************************/
int labelFile( FILE *fh, char *proc, char *file ){

    char *level;
	element *new;
	map *proc_map = NULL, *file_map = NULL;
	
	/* make mapping node for new file */
	map *m = (map *) malloc(sizeof(map));
	m->len = strlen( file );
	m->name = (char *) malloc( m->len );
	strncpy( m->name, file, m->len );
	
	/* find if a label mapping with filename-prefix */
	element *fileMatch = get( &label_mapping, file, &matchMapName );
	if ( fileMatch ) 
		file_map = (map *)fileMatch->data;

	/* find label mapping for creating process if any */
	element *procMatch = ( proc ? get( &system_mapping, proc, matchMapName ) : NULL );
	
	if ( procMatch ) 
		proc_map = (map *)procMatch->data;
	/* use lowest level of two: creating process and filename-prefix */
	if ( proc_map )
	{
		if ( file_map ) {
			int proc_level = pos( &lattice, proc_map->l->name, &matchLevelName);
			int file_level = pos( &lattice, file_map->l->name, &matchLevelName);
			if ( proc_level <= file_level ) {
				m->l = proc_map->l;
			}
			else {
				m->l = file_map->l;
			}
		}
		else {
			m->l = proc_map->l;
		}
	}
	else if ( file_map ) {
			m->l = file_map->l;
	}
	else {
		fprintf( fh, "ERROR labelFile[t%d]: no mapping for process: %s\n", cmdCt, proc );
		return -1;
	}

	/* add elt to system mappings */
	new = (element *) malloc(sizeof(element));
	new->type = E_MAP;
	new->data = (void *)m;
	insert( &system_mapping, new, NULL );
	
	/* log result */
	level = (char *) malloc( m->l->len+1 );
	strncpy( level, m->l->name, m->l->len );
	level[m->l->len] = 0;
	fprintf( fh, "labelFile[t%d]: Setting label successful for file %s to %s\n", cmdCt, file, level );
	free( level );

	return 0;	
}



/**********************************************************************

    Function    : labelProcess
    Description : Compute label (level) for process
    Inputs      : fh - output file pointer
                  name - user name or parent process name
                  proc - process name
                  mapping - label or system mapping for finding level for name
    Outputs     : 0 if successfully completed, -1 if failure

***********************************************************************/
int labelProcess( FILE *fh, char *name, char *proc, list *mapping ){
    char *level;
	element *new;

	/* make mapping node */
	map *m;
	/* YOUR CODE GOES HERE */
	m=(map *) malloc(sizeof(map));
	m->len = strlen( proc );
	m->name = (char *) malloc( m->len );
	strncpy( m->name, proc, m->len );
	/* use creating process level for the mapping node */
	/* YOUR CODE GOES HERE */
	map *name_map = NULL;
	element *nameMatch = get( mapping, name, &matchMapName );
	if ( nameMatch ){ 
		name_map = (map *)nameMatch->data;
		//m->l = name_map->l;
		m->l=(struct level_t *) malloc(sizeof(struct level_t));
		m->l->len=name_map->l->len;
		m->l->name=(char *) malloc( name_map->l->len );
		strncpy( m->l->name, name_map->l->name, m->l->len );
	}
	 if( /* Condition when the required mapping is not found */ /* YOUR CODE GOES HERE */ nameMatch==NULL)
	{
		fprintf( fh, "ERROR labelProcess[t%d]: no mapping for name: %s\n", cmdCt, name );
		return -1;
	}

	/* add element to system mappings */
	/* YOUR CODE GOES HERE */
	new = (element *) malloc(sizeof(element));
	new->type = E_MAP;
	new->data = (void *)m;
	insert( &system_mapping, new, NULL );

	/* log result */
	level = (char *) malloc( m->l->len+1 );
	strncpy( level, m->l->name, m->l->len );
	level[m->l->len] = 0;
	fprintf( fh, "labelProcess[t%d]: Setting label successful for process %s to %s\n", cmdCt, proc, level );
	free( level );

	return 0;
}



/**********************************************************************

    Function    : checkAccess
    Description : Authorize op
    Inputs      : fh - output file pointer
                  proc - name of process
                  file - name of file accessed
                  op - identity of operation (some combination of rwx)
    Outputs     : 0 if successfully completed, -1 if failure

***********************************************************************/
int checkAccess( FILE *fh, char *proc, char *file, int op ){

    /* get process level and file level */
	element *procElt;	
	element *fileElt;	
	/* YOUR CODE GOES HERE */
	fileElt = get( &system_mapping, file, &matchMapName );
	procElt = get( &system_mapping, proc, &matchMapName );
	/* check whether operation is authorized for those level */
	if ( !procElt || !fileElt ) 
	{
		fprintf( fh, "ERROR checkAccess[t%d]: no mapping for proc 0x%p or file 0x%p\n", cmdCt, procElt, fileElt );
		return -1;
	}
	
	/* extract mapping from elt */
	map *proc_map = (map *)(procElt->data);
	map *file_map = (map *)(fileElt->data);

	/* get level index of process and file for authorization */
	int proc_level;
	int file_level;
	/* YOUR CODE GOES HERE */

	proc_level = pos( &lattice, proc_map->l->name, &matchLevelName );
	file_level = pos( &lattice, file_map->l->name, &matchLevelName );
	if (( proc_level < 0 ) || ( file_level < 0 )) {
		fprintf( fh, "ERROR checkAccess[t%d]: labeling problem for process %s or file %s\n", cmdCt, proc, file );
		return -1;
	}

	/* process level must be dominated or equal to file level to read/exec */
	if ( op & (O_READ | O_EXEC) ) {
		if ( /* YOUR CODE GOES HERE */ proc_level > file_level) {  /* can't read */
			if (!( mic && (!( op & O_EXEC )))) { /* still allow for read in mic */
				fprintf( fh, "checkAccess[t%d]: DENY! %s: process %s for file %s\n", cmdCt, proc, 
					 (( ( op & ( O_READ | O_EXEC )) == ( O_READ | O_EXEC )) ? "read/exec" : 
					  (( ( op & ( O_READ | O_EXEC )) == O_READ ) ? "read" : "exec")), file );
				return -1;
			}
		}
		/* log result */
		fprintf( fh, "checkAccess[t%d]: ALLOW! proc: %s may %s on file: %s\n", cmdCt, proc, 
			 (( ( op & ( O_READ | O_EXEC )) == ( O_READ | O_EXEC )) ? "read/exec" : 
			  (( ( op & ( O_READ | O_EXEC )) == O_READ ) ? "read" : "exec")), file );
	}

	/* file level must be dominated or equal to process level to write */
	if ( op & O_WRITE ) {
		 if ( /* YOUR CODE GOES HERE */proc_level < file_level ) {  /* can't write */
			fprintf( fh, "checkAccess[t%d]: DENY! write: process %s for file %s\n", cmdCt, proc, file );
			return -1;
		}
		/* log result */
		fprintf( fh, "checkAccess[t%d]: ALLOW! proc: %s may write file: %s\n", cmdCt, proc, file );
	}
	return 0;
}



/**********************************************************************

    Function    : checkTrans
    Description : Perform transition of levels if policy requires
    Inputs      : fh - output file pointer
                  proc - name of process
                  file - name of file accessed
                  op - operation being performed
                  ttype - transition type (process or file)
    Outputs     : 0 if successfully completed, -1 if failure

***********************************************************************/
int checkTrans( FILE *fh, char *proc, char *file, int op, int ttype ){
     /* get process level and file level */
	element *procElt;	
	element *fileElt;	
	/* YOUR CODE GOES HERE */
	fileElt = get( &system_mapping, file, &matchMapName );
	procElt = get( &system_mapping, proc, &matchMapName );
	/* check whether operation is authorized for those level */
	if ( !procElt || !fileElt ) 
	{
		fprintf( fh, "ERROR checkTrans[t%d]: no mapping for proc 0x%p or file 0x%p\n", cmdCt, procElt, fileElt );
		return -1;
	}

	/* extract mapping from elt */
	map *proc_map = (map *)(procElt->data);
	map *file_map = (map *)(fileElt->data);

	/* build trans objects to see if one exists */
	trans *t = (trans *)malloc(sizeof(trans));
	t->subj = proc_map->l;
	t->obj = file_map->l;
	int i;
	for ( i = 0; i < MAX_OPS; i++ ) {
		int testop = (1 << i);
		t->op = testop;

		/* if op is applicable, see if trans for that combo of proc, file, op */
		if ( op & testop ) {
			/* YOUR CODE GOES HERE */
			element *transElt= get( &trans_mapping, t, matchTrans );
			if (!transElt ) {  /* can't write */
			
				return -1;
			}
			trans *apply = (trans *)(transElt->data);
			apply->new= (ttype == T_PROC) ? file_map->l : proc_map->l;

			if (ttype == T_PROC)
				proc_map->l=file_map->l;
			else
				file_map->l=proc_map->l;
			/* log result */
			char *level = (char *) malloc( apply->new->len+1 );
			strncpy( level, apply->new->name, apply->new->len );
			level[apply->new->len] = 0;
			fprintf( fh, "checkTrans[t%d]: trans %s %s to %s\n", cmdCt,
				 ((ttype & T_PROC) ? "PROCESS" : "FILE" ), 
				 ((ttype & T_PROC) ? proc : file ), level );
			free( level );
		}
	}

	free( t );
	return 0;
}



/**********************************************************************

    Function    : policyCmds
    Description : Process policy commands 
    Inputs      : fh - output file pointer
                  content - command content
                  len - length of command content
    Outputs     : cmdCt if successfully completed, -1 if failure

***********************************************************************/
int policyCmds( FILE *fh, char *content, int len ){
    fprintf( fh, "==================== Parsing Policy Commands ====================\n");
	char *arg1, *arg2, *arg3, *line;
	cmdCt = 1;

	/* Iterate through the content line-by-line */
	line = strtok( content, "\n" );
	while ( line != NULL )
	{
		arg1 = (char *)malloc(strlen(line));
		arg2 = (char *)malloc(strlen(line));
		arg3 = (char *)malloc(strlen(line));

		/* construct policy */
		/* arg1: lower level; arg2: higher level */
		if ( sscanf( line, "pstate > %s %s", arg1, arg2 ) == 2 )
		{
			addLattice( fh, arg1, arg2 );
		}

		/* arg1: filename-prefix; arg2: level */
		else if ( sscanf( line, "lstate file-old %s %s", arg1, arg2 ) == 2 )
		{
			addLabelPolicy( fh, arg1, arg2 );
		}

		/* arg1: filename-prefix; arg2: level */
		else if ( sscanf( line, "lstate file-new %s %s", arg1, arg2 ) == 2 )
		{
			addLabelPolicy( fh, arg1, arg2 );
		}

		/* arg1: username-prefix; arg2: level */
		else if ( sscanf( line, "lstate user-new %s %s", arg1, arg2 ) == 2 )
		{
			addLabelPolicy( fh, arg1, arg2 );
		}

		/* arg1: subject level; arg2: object level; arg3: resultant level (process) */
		else if ( sscanf( line, "tstate proc-exec %s %s %s", arg1, arg2, arg3 ) == 3 )
		{
			addTransPolicy( fh, arg1, arg2, arg3, O_EXEC, T_PROC );
		}

		/* arg1: subject level; arg2: object level; arg3: resultant level (process) */
		else if ( sscanf( line, "tstate open-read %s %s %s", arg1, arg2, arg3 ) == 3 )
		{
			addTransPolicy( fh, arg1, arg2, arg3, O_READ, T_PROC );
		}

		/* arg1: subject level; arg2: object level; arg3: resultant level (file) */
		else if ( sscanf( line, "tstate open-write %s %s %s", arg1, arg2, arg3 ) == 3 )
		{
			addTransPolicy( fh, arg1, arg2, arg3, O_WRITE, T_FILE );
		}
		
		/* set MIC enforcement - read down is OK */
		else if ( sscanf( line, "policy %s", arg1 ) == 1 )
		{
			mic = 1;
		}

		else 
		{
			fprintf( fh, "Policy Command failure: %s\n", line );
			return -1;
		}

		free(arg1), free(arg2), free(arg3);
        /* Iterate through the content line-by-line */
		line = strtok( NULL, "\n" );
		cmdCt++;
	}
		
	fprintf( fh, "\n==================== Parsing Policy was Successful ====================\n\n");
	return cmdCt;
}



/**********************************************************************

    Function    : execCmds
    Description : Process exec commands 
    Inputs      : fh - output file pointer
                  content - command content
                  len - length of command content
    Outputs     : cmdCt if successfully completed, -1 if failure

***********************************************************************/
int execCmds( FILE *fh, char *content, int len ){
    fprintf( fh, "==================== Parsing Test Execution Commands ====================\n");
	char *arg1, *arg2, *line;
	cmdCt = 1;

    /* Iterate through the content line-by-line */
	line = strtok( content, "\n" );
	while ( line != NULL )
	{
		arg1 = (char *) malloc(strlen(line));
		arg2 = (char *) malloc(strlen(line));

		/* perform commands */
		/* label existing files based on name (arg1) */
		if ( sscanf( line, "old-file %s", arg1 ) == 1 )
		{
			labelFile( fh, NULL, arg1 );
		}

		/* label existing files based on name (arg2) or process (arg1) if no rule for name */
		else if ( sscanf( line, "create %s %s", arg1, arg2 ) == 2 )
		{
			labelFile( fh, arg1, arg2 );
		}
		/* label login process (name in arg2) based on username (arg1) */
		else if ( sscanf( line, "login %s %s", arg1, arg2 ) == 2 )
		{
			labelProcess( fh, arg1, arg2, &label_mapping);
		}

		/* label forked process (name in arg2) based on parent (arg1) */
		else if ( sscanf( line, "fork %s %s", arg1, arg2 ) == 2 )
		{
			labelProcess( fh, arg1, arg2, &system_mapping );
		}
		else if ( sscanf( line, "exec %s %s", arg1, arg2 ) == 2 )
		{
			checkTrans( fh, arg1, arg2, O_EXEC, T_PROC );
			checkAccess( fh, arg1, arg2, O_EXEC );
		}
		/* can proc (arg1) read file (arg2)?  check for transition of proc */
		else if ( sscanf( line, "open-read %s %s", arg1, arg2 ) == 2 )
		{
			checkTrans( fh, arg1, arg2, O_READ, T_PROC );
			checkAccess( fh, arg1, arg2, O_READ );
		}

		/* can proc (arg1) write file (arg2)?  check for transition of file */
		else if ( sscanf( line, "open-write %s %s", arg1, arg2 ) == 2 )
		{
			checkTrans( fh, arg1, arg2, O_WRITE, T_FILE );
			checkAccess( fh, arg1, arg2, O_WRITE );
		}

		/* can proc (arg1) read-write file (arg2)?  check for transitions of file and proc */
		else if ( sscanf( line, "open-rw %s %s", arg1, arg2 ) == 2 )
		{
			checkTrans( fh, arg1, arg2, O_READ, T_PROC );
			checkTrans( fh, arg1, arg2, O_WRITE, T_FILE );
			checkAccess( fh, arg1, arg2, O_READ | O_WRITE );
		}
		
		else if ( sscanf( line, "read %s %s", arg1, arg2 ) == 2 )
		{
			checkAccess( fh, arg1, arg2, O_READ );
		}

		/* can proc (arg1) write file (arg2)? */
		else if ( sscanf( line, "write %s %s", arg1, arg2 ) == 2 )
		{
			checkAccess( fh, arg1, arg2, O_WRITE );
		}
		
		else 
		{
		    if( line[0] != '#' ){
			fprintf( fh, "Test Execution Command failure!! Inavlid Command %d: %s\n", cmdCt, line );
		    }
		}

		free(arg1), free(arg2);
        /* Iterate through the content line-by-line */
		line = strtok( NULL, "\n" );
		cmdCt++;
	}
		
	fprintf( fh, "\n==================== Test Execution was Successful ====================\n\n");
	return cmdCt;
}



/**********************************************************************

    Function    : main
    Description : Runs the reference monitor on the entire test suite. 
    Usage : ./cse543-p2 </path/to/policy-file> </path/to/test-case-file> </path/to/outfile>

***********************************************************************/
int main( int argc, char *argv[] ){
    int len1, status1, len2, status2;
    char *policy_content;
    char *test_execution_content;
	FILE *fh;

	/* initialize linked lists */
	init( &system_mapping );
	init( &lattice );
	init( &label_mapping );
	init( &trans_mapping );

	if ( argc < 4 ) {
		printf("Usage: ./cse543-p2 </path/to/policy-file> </path/to/test-case-file> </path/to/outfile>\n");
		exit(-1);
	}

    /* OPEN the output file in write mode */
	if ( (fh=fopen( argv[3], "w" )) == NULL ){
		printf("main function: failure opening output file [%.64s]\n", argv[3]);
		exit(-1);
	}

    /* OPEN policy commands file */
	len1 = read_from_file( argv[1], &policy_content );
	if ( len1 <= 0 ) {
		printf("main function: failed policy file read: %s\n", argv[1]);
		goto end;
	}

    /* OPEN test execution commands file */
	len2 = read_from_file( argv[2], &test_execution_content );
	if ( len2 <= 0 ) {
		printf("main function: failed test case file read: %s\n", argv[2]);
		goto end;
	}

    /* RUN policy commands */
	status1 = policyCmds( fh, policy_content, len1 );
	if ( status1 < 0 )
	{
		printf("main: failure on running policy commands: %s\n", argv[1] );
		goto end;
	}

    /* RUN test execution commands */
	status2 = execCmds( fh, test_execution_content, len2 );
	if ( status2 < 0 )
	{
		printf("main: failure on running test execution commands: %s\n", argv[1] );
		goto end;
	}

end:
	fclose( fh );
    return 0; 
}
