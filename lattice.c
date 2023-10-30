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
list lattice;
list label_mapping;
list trans_mapping;


/**********************************************************************

    Function    : addLattice
    Description : Add levels - add from low to high levels
    Inputs      : fh - output file pointer
                  low - name of lower level
                  high - name of higher level
    Outputs     : 0 if successfully completed, -1 if failure

***********************************************************************/
int addLattice( FILE *fh, char *low, char *high ){

    /* Check if low level already exists in lattice */	
	element *lowElt = get( &lattice, low, &matchLevelName );
	/* if no low already, must make one */
	if ( !lowElt ) {
		/* make new level for low */
		level *l = (level *) malloc(sizeof(level));
		l->len = strlen( low );
		l->name = (char *) malloc( l->len );
		strncpy( l->name, low, l->len );

		/* make new element for low in lattice */
		lowElt = (element *) malloc(sizeof(element));
		lowElt->type = E_LEVEL;
		lowElt->data = (void *)l;
		lowElt->next = NULL;

		/* add to lattice */
		insert( &lattice, lowElt, NULL );
		fprintf( fh, "AddLattice[p%d]: level: %s \n", cmdCt, low );
	}

    /* Check if high level already exists in lattice */	
	element *highElt = get( &lattice, high, &matchLevelName );
	/* if no high already, must make one */
	if ( !highElt ) {
        /* make new level for high */
        level *l = (level *) malloc(sizeof(level));
        l->len = strlen( high );
        l->name = (char *) malloc( l->len );
        strncpy( l->name, high, l->len );

        /* make new element for high in lattice */
        highElt = (element *) malloc(sizeof(element));
        highElt->type = E_LEVEL;
        highElt->data = (void *)l;
        highElt->next = NULL; 

        /* add to lattice */
        insert( &lattice, highElt, lowElt );
        fprintf( fh, "AddLattice[p%d]: level: %s > %s \n", cmdCt, high, low );
    }

    return 0;
}



/**********************************************************************

    Function    : addLabelPolicy
    Description : Add mapping from name to level
    Inputs      : fh - output file pointer
                  name - name (prefix) of objects at level
                  level - name of level
    Outputs     : 0 if successfully completed, -1 if failure

***********************************************************************/
int addLabelPolicy( FILE *fh, char *name, char *level ){

    element *new;

	/* hint - find lattice level for map */
	/* YOUR CODE GOES HERE */
	element *ll = get( &lattice, level, &matchLevelName );
	if (!ll) {
		fprintf( fh, "ERROR AddLabelPolicy[p%d]: level not found.\n", cmdCt);
		return -1;
	}
	struct level_t *tl= (struct level_t*)ll->data;
	
    /* hint - make mapping node */
	/* YOUR CODE GOES HERE */
	map *m = (map *) malloc(sizeof(map));
        m->len = strlen( name );
        m->name = (char *) malloc( m->len );
        strncpy( m->name, name, m->len );
	m->l=(struct level_t *) malloc(sizeof(struct level_t));
	m->l->len=tl->len;
	m->l->name=(char *) malloc( tl->len );
	strncpy( m->l->name, tl->name, tl->len );
	/* hint - create element for label_mapping */
	/* YOUR CODE GOES HERE */
	new = (element *) malloc(sizeof(element));
        new->type = E_MAP;
        new->data = (void *)m;
        new->next = NULL;
    /* add the above element to label mappings */ 
	insert( &label_mapping, new, NULL );	
	fprintf( fh, "AddLabelPolicy[p%d]: Add mapping for name %s to level %s\n", cmdCt, name, level );	
	return 0;
}



/**********************************************************************

    Function    : addTransPolicy
    Description : Add mapping from name to level
    Inputs      : fh - output file pointer
                  subj_level - level of subject of op
                  obj_level - level of object of op
                  new_level - resultant level of subj or obj (op-dependent)
                  op - operation (rwx)
                  ttype - transition type (process or file)
    Outputs     : 0 if successfully completed, -1 if failure

***********************************************************************/
int addTransPolicy( FILE *fh, char *subj_level, char *obj_level, char* new_level, int op, int ttype ){

    element *new;
	
	/* get subject, object, new levels */
	/* YOUR CODE GOES HERE */
	element *se = get( &lattice, subj_level, &matchLevelName );
	element *oe = get( &lattice, obj_level, &matchLevelName );
	element *ne = get( &lattice, new_level, &matchLevelName );
	/* check if any level is missing */
	if ( /* Condition when any level is not found */ /* YOUR CODE GOES HERE */ !se || !oe || !ne) {
		fprintf( fh, "ERROR AddTransPolicy[p%d]: One/More levels not found for mapping.\n", cmdCt);
		return -1;
	}

	/* make trans node */
	/* YOUR CODE GOES HERE */
	trans *t = (trans *) malloc(sizeof(trans));
	
	struct level_t *sl= (struct level_t*)se->data;
	struct level_t *ol= (struct level_t*)oe->data;
	struct level_t *nl= (struct level_t*)ne->data;
        t->subj = sl;
        t->obj = ol;
	t->new = nl;
        t->op = op;
	
	/* create element for trans mapping */
	/* YOUR CODE GOES HERE */	
	new = (element *) malloc(sizeof(element));
        new->type = E_TRANS;
        new->data = (void *)t;
        new->next = NULL;

    /* add to trans mapping */
	insert( &trans_mapping, new, NULL );
	fprintf( fh, "AddTransPolicy[p%d]: on %s, if subject level is %s and object level is %s, \n\t\t\t then transition %s to level %s\n", cmdCt, 
                    (( op == O_EXEC ) ? "EXEC" : (( op == O_READ ) ? "READ" : "WRITE" )), subj_level, obj_level, (( ttype == T_PROC ) ? "PROCESS" : "FILE"), new_level );

	return 0;
}



/**********************************************************************

    Function    : Match Functions
    Description : Find if node name matches supplied object
    Inputs      : element - element to check 
                  obj - object to match on
    Outputs     : 0 if successfully completed, -1 if failure

***********************************************************************/

//Matches using Level Name
int matchLevelName( element *e, void *obj ){

	if ( strncmp( (char *)obj, ((level *)(e->data))->name, ((level *)(e->data))->len ) == 0 ) 
		return 1;
	return 0;
}

//Matches using Map Name
int matchMapName( element *e, void *obj ){

    if ( strncmp( (char *)obj, ((map *)(e->data))->name, ((map *)(e->data))->len ) == 0 ) 
		return 1;
	return 0;
}

//Matches using Level Object
int matchLevel( element *e, void *obj ){

    return( ((level *)(e->data)) == (level *)obj );
}

//Matches using Trans Object
int matchTrans( element *e, void *obj ){

    trans *t1, *t2;
	t1 = (trans *)e->data;
	t2 = (trans *)obj;
	return( (strcmp(t1->subj->name, t2->subj->name)==0) &&
		(strcmp(t1->obj->name, t2->obj->name)==0) &&
		(t1->op == t2->op) );
}
