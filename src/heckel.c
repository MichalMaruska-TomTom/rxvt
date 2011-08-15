
#include "../config.h"		/* NECESSARY */
#define INTERN_HECKEL
#include "rxvt.h"		/* NECESSARY */

#include <X11/Xmd.h>		/* get the typedef for CARD32 */
#include <stdlib.h>             /* mmc: logging stderr */


/* --------------------------------------------------------- Heckel ---------------------------------- */

typedef struct
{
    int hash;			/* hashed value of token sequence */
    int other;			/* line number of equivalent line in other file, or 0 if not matched */
} array_entry;


#define MAX_OTHER  5

/* mmc: Given a string (line) we construct a hash, and
 * From the hash we obtain (how?) this info-node:
 *   Here we have a list of line numbers in the 2 buffers. We keep a count of the 2 lists.
 *   `key' is the hash value. */
typedef struct hash_node {
	struct hash_node *next_result; /* next in the same bucket! */
	int num_current;
	int old[MAX_OTHER];

	int num_desired;
	int new[MAX_OTHER];
	int key;                /* This is the hash. an int representing the string/line-text. */
} hash_type;




typedef struct _hash_table 
{
  int num_buckets;
  hash_type** buckets;
} hash_table;

#include "heckel.intpro"	/* PROTOS for internal routines */




#define newplus(t,n)	(struct t*)malloc(sizeof(struct t) + n)

/* mmc:
 * lookup in Hash. If not present, add (at the head of the same-hash chain), a hash_node w/ 2 more bytes:  */
/* INTPROTO */
hash_type*
find_symtab(hash_table* hash,int s)
{
        /* fixme: make Hash an argument. */
    hash_type* h;                /* key, next_result */
    int s1;
    s1 = s % hash->num_buckets;

    /* fprintf(stderr, "%s: bucket %d", __FUNCTION__, s1); */
    h = hash->buckets[s1];
    while (h != NULL)
	{
            if (s == h->key)
                return h;

            else h = h->next_result;
	}

    /* Not found */
    h = newplus(hash_node, 2);  /* why 2 more bytes? */
    h->next_result = hash->buckets[s1];
    hash->buckets[s1] = h;
    /* added at head */

    h->key = s;
    h->num_desired = 0;
    h->num_current = 0;
    return h;
}

/*   |--------------------------------|  array
 *     |
 *     v
 *     s-link
 */
/* INTPROTO */
void
clear_hash_table(hash_table *hash) 
{
    int i;
    hash_type* temp_ptr1;
    hash_type* temp_ptr2;

    for (i = 0 ; i < hash->num_buckets ; i++)
	{
            temp_ptr1 = hash->buckets[i];
            while (temp_ptr1 != NULL)
		{
                    temp_ptr2 = temp_ptr1;
                    temp_ptr1 = temp_ptr1->next_result;
                    free(temp_ptr2);
		}
            hash->buckets[i] = NULL;
	}
    hash->num_buckets = 0;
}



#define DEBUG_attach 0
/* Walks master array.  Through the hashtable, find the points-of-equality, and `attach' them.
 * (link original lines in files.)
 */
/* INTPROTO */
void
attach_corresponding(int num_lines, array_entry* current, array_entry* desired, hash_table *hash)
{
    int line;
    for (line = 0 ; line < num_lines ; line++) { /* fixme: why 1 ?? */
        hash_type* value = find_symtab(hash, current[line].hash);

        /* same number of lines w/ that hash: */
        if ((value->num_current == value->num_desired)
            && (value->num_current) && (value->num_current <= MAX_OTHER))
            /* mmc: why check upper limit?    I would use  <    !!! */
            {
                int i;
#if DEBUG_attach
                if (value->num_current > 1)
                    fprintf(stderr, "attaching more %d points-of-equality!\n", value->num_current);
#endif
                
                for (i = 0; (i < value->num_desired) ; i++) {
                    /* fixme: */
                    if ((desired[value->new[i]].other == -1) && (current[value->old[i]].other == -1))
                        /* mmc: at the beginning .other = 0! */
                        {
                            /* bind: */
#if DEBUG_attach
                            fprintf(stderr, "attaching current %d -> new %d!\n",
                                    value->old[i], value->new[i]);
#endif

                            current[value->old[i]].other = value->new[i]; /* the row */
                            desired[value->new[i]].other = value->old[i];



                            /* sentinel? */
                            if ((value->new[i] >= MAX_OTHER) /* how is this related ? */
                                && (value->old[i] >= MAX_OTHER) /*  */

                                /* fixme:  1-based!  */
                                && ( current[value->old[i]-1].other == -1)
                                && ( desired[value->new[i]-1].other == -1))
                                {/* link previous array entry */
#if 0                                    
                                    fprintf(stderr, "???????\n");
#endif
                                    current[value->old[i]-1].other = value->new[i]-1;
                                    desired[value->new[i]-1].other = value->old[i]-1;

                                    if ((! desired[value->new[i]-2].other) && (! current[value->old[i]-2].other))
                                        {
                                            current[value->old[i]-2].other = value->new[i]-2;
                                            desired[value->new[i]-2].other = value->old[i]-2;
                                        }
                                }
                        }
                }
            }
                    
        else
            {
#if DEBUG_attach
                fprintf(stderr, "cannot attach line %d: %d vs. %d.\n",
                        line, value->num_current, value->num_desired);
#endif
            }
    }
}


/* extend links downwards */
/* INTPROTO */
void
extend_downwards(int num_lines, array_entry* current, array_entry* desired)
{
    int line;
    for (line = 0 ; line < num_lines ; line++) {
        if (( current[line].other > -1) /* something matched */
            /* fixme:  here I'd benefir from `start'=1 */
            && (desired[current[line].other + 1].other == -1)
            && (current[line + 1].other  == -1)

            && (current[line + 1].hash == desired[current[line].other + 1].hash )) /* step 1 above */
            {
#if DEBUG_attach
                fprintf(stderr, "vvvv found extension!\n");
#endif
                /* bind them: */
                desired[current[line].other + 1].other = line + 1;
                current[line + 1].other = current[line].other + 1 ;
            }
    }
}

/* extend links upwards */
/* INTPROTO */
void
extend_upwards(int num_lines, array_entry* current, array_entry* desired)
{
    int line;
    /* fixme: num_lines is sXXX, 0 ? */
    for (line = num_lines ; line > 0 ; line--) { /* 0 or  */
        
        if ((current[line].other > 1)
            && (current[line - 1].hash == desired[current[line].other - 1].hash)
            /* fixme: */
            && (desired[current[line].other - 1].other == -1))
            {
#if DEBUG_attach
                fprintf(stderr, "^^^^ found extension!\n");
#endif 
                desired[current[line].other - 1].other = line - 1;
                current[line - 1].other = current[line].other - 1 ;


                /* don't check  hashes??  are they fake anyway? */
                
                if ((current[line].other > MAX_OTHER)  /* sentinel ? */
                    && (line > MAX_OTHER) /* mmc: ???   */
                    && (! desired[current[line].other - 2].other)
                    && (! current[line - 2].other))
                    {
                        /* link previous array entry */
                        desired[current[line].other - 2].other = line - 2;
                        current[line - 2].other = current[line].other - 2;

                        if ((! current[line - 3].other) && (! desired[current[line].other - 3].other))
                            {
                                desired[current[line].other - 3].other = line - 3;
                                current[line - 3].other = current[line].other - 3;
                            }
                    }
            }
    }
}


/* inline static */
/* INTPROTO */
int
hash_byte_vector(const text_t* vector, int len) /* unsigned? */
/* fixme: bug vector = 0 !! */
{
    int sum = 0;
    /* With Duff's device ? */
    int i;
    for (i = 0 ; i < len; i++)
        sum += vector[i];       /* sizeof(text_t) ? */
#if 0
    fprintf(stderr, "hash %d: %.80s (%d)\n", sum, vector, len);
#endif
    return sum;
}



/* So, we have 2 equal-size buffers,  r->screen (or ->swap, or ->snapshot) and r->drawn_text.
 * We want to run heckel on them, and get:  scolling-index and a bitmap of changed rows
 * (vs. simply scrolled rows) */
 /* len[] */
/* INTPROTO */
void
construct_array_and_hash(hash_table *hash, int num_lines, text_t** buffer, int len,
                         array_entry* row_table, int current_p)
{
    char *beginning = "BEGINNIN";
    
    /* global state:  Should I really hash some string? */
#if HASH3
    int hash1 = 27;
    int hash2 = 10568;
#endif
    int hash3;
    
    /* there is an upper bound for the # of lines read:  they have to be stored in a hash: */
    int line;
    for (line = 0; line < num_lines; line++)
	{
            /* the hash is of the 3 consecutive lines? */
            hash3 = hash_byte_vector(buffer[line], len); /* fixme: Should walk ints rather than char?   [line]*/

            int hashed;
#if HASH3
            hashed = 5* hash3 + 3* hash2 + hash1;
            /* Shift */
            hash1 = hash2;
            hash2 = hash3;
#else
            hashed = hash3;
#endif

#if 0
            printf("read: hashed %d:  %s\n", hashed, str3);
#endif

            /* fixme! I changed the hash function to not consider other, neighbour lines */
            hash_type* value = find_symtab(hash, hashed); /* hashed */
            if (current_p) {
                if (value->num_current < MAX_OTHER)
                    value->old[value->num_current] = line; /*  */
                /* is this a bug? */
                value->num_current++;
            } else {
                if (value->num_desired < MAX_OTHER)
                    value->new[value->num_desired] = line;
                value->num_desired++;
            }
            
            row_table[line].hash = hashed; /* hashed */
            row_table[line].other = -1;
	}
    
    row_table[num_lines].hash = 734;        /* ??? */
#if DEBUG
    printf("%s: read %d rows\n", __FUNCTION__, line - 1);
#endif
}



/* Analysis Procedure:   both arrays are the same lenght! */
/* INTPROTO */
int
analyse(const unsigned int num_lines, array_entry* array, array_entry* copy_array)
{
    /* Scan & keep state: */
#if 0
  int same = 0;  /*not interesting!*/
  int file1size = 0;
  int file2size = 0;
  int max;
#endif

  int i;
  int block = 0;

#define MAX_SHIFT 10 
  unsigned int shifts[2*MAX_SHIFT -1 ] = {0}; /* This seems suspect!    -1 0 1   max is 2*/
  bzero(shifts, (2*MAX_SHIFT - 1) * sizeof(int));
  
  
  for (i = 0 ; i < num_lines ; i++) /* we start from 1 not 0 ?? */
    if (array[i].other > -1)        /* fixme:  0 ? */
        {
#if 0
            fprintf(stderr, "Shift on %d is: %d!\n", i, (array[i].other - i));
#endif
            /* same++;*/
            if (abs(array[i].other - i) < MAX_SHIFT) /*  */
                ++shifts[(array[i].other - i) + MAX_SHIFT -1]; /* fixme! */

            if ((i > 0)
                /* Still the same shift? */
                && ((array[i].other - i) == (array[i - 1].other - (i - 1))))
                /* abs( array[i].other - array[i-1].other) == 1 */
                {
                    block++;
                }
            else
                {
#if 0
                    if (i>0)
                        fprintf(stderr, "%d block at %d, and another started!\n", block, i);
#endif
                    block = 1;
                }
        } else {
        /* This row is new! */
        /*uniqueN++;*/
        if (block)
            block = 0;
    }
  {
      int i;
      int best = 0;
      unsigned int needed = (num_lines - 2) * 8/10; /* leave top & bottom lines as constants!  */
      int maximum = 0;
      
#if 0
      fprintf(stderr, "Shifts: %u, needed %u\n", num_lines, needed);
#endif
      
    /* This is wrong:   i = - MAX_SHIFT+1 .... MAX_SHIFT -1 */
      for(i = 0; i< 2* MAX_SHIFT -1; i++)
          if (shifts[i])
              {
                  if (shifts[i] > maximum)
                      maximum = shifts[i];
                  if (shifts[i] >= needed)
                      {
                          best = i - MAX_SHIFT + 1; /*  ?? */
#if 0
                          fprintf(stderr, "Bingo! %d\n", best);
#endif 
                      }
#if 0
                  fprintf(stderr, "%d: %u\n", i - MAX_SHIFT + 1, shifts[i]);
#endif
              }

#if 0
      /* if (debug) */
      if (! best)
          fprintf(stderr, "best is %u, needed was %u\n", maximum, needed);
#endif
      return best;
  }
}





/* Given NUM_LINES text lines, i.e. arrays of char* pointners long LINE_LEN
 * .... in fact 2 of such text buffers BUFFER and COPY
 * return a number --- possible shift which applied to BUFFER will turn it (bring similararity)
 * to COPY
 *
 * CHANGED will be filled with 0/1 to signal if a row/line is changed between COPY and shifted BUFFER */

/* r->screen->rend */
/* heckel(
 *   heckel (r->TermWin.nrow, r->drawn_text, r->screen+ offset,  int** changed);
 *
 * */

/* buffer Is what we have now, copy is what we need  */
                         /* int len[], int copy_len[], */

/* EXTPROTO */
int
heckel (int num_lines,int line_len, text_t** buffer, text_t** copy,  char* changed)
{

    /* prepare the hashes:  */
    hash_table hash;
    hash.num_buckets = 1001;    /* MAX_HASH1 */
    {
        int bucket_len = sizeof(hash_type*) * hash.num_buckets;
        hash.buckets = (hash_type**) alloca(bucket_len);
        bzero(hash.buckets, bucket_len);
    }
    /* hash entries allocated with malloc! */


    /* on the stack? */
    array_entry* array = alloca((num_lines + 1) * sizeof(array_entry));
    array_entry* copy_array = alloca((num_lines + 1) * sizeof(array_entry));

#if 0
    fprintf(stderr, "3: %.80s\n", buffer[3]);
    fprintf(stderr, "1: %.80s\n", copy[1]);
    if (strncmp(buffer[3], copy[1],80) == 0)
        fprintf(stderr,"EQUAL!\n");
#endif
        
    construct_array_and_hash (&hash, num_lines, buffer, line_len, array, 1); /* len */

    construct_array_and_hash (&hash, num_lines, copy, line_len, copy_array, 0); /* copy_len */

    /* sentinal at end of arrays */
    {
        hash_type* h = find_symtab(&hash, 734);

        h->num_current = 1;
        h->num_desired = 1;

        h->old[0] = num_lines;
        h->new[0] = num_lines;

        /* note, that we have already set this! in `construct_array_and_hash' */
        array[num_lines].other = num_lines;
        copy_array[num_lines].other = num_lines;
    }
    
    /* run the heckel: construct Shift arrays */
    attach_corresponding(num_lines, array, copy_array, &hash);
    extend_downwards(num_lines, array, copy_array);
    extend_upwards(num_lines, array, copy_array);

    
    /* Scan the Shift array, and detect scrolling-index */
    /* make the bitmap */
    {
        int shift = analyse(num_lines, array, copy_array);
        clear_hash_table(&hash);
        if (shift)
            {
                int i;
                for(i = 0; i< num_lines; i++)
                    {
                        /* fixme:  array[i].other != 0     It seems i will have to shift all indexes by one.
                         * Or use -1 to mean no other exists? */
                        if ((array[i].other != -1)
                            && (array[i].other - i == shift))
                            {
                                /* if (array[i].other  */
                                changed[i] = 0; /* 1 -> don't scroll it! */
                            }
                        else
                            {
#if DEBUG_heckel
                                fprintf(stderr, "%d is changed, or scrolls differently %d %d\n",
                                        i, array[i].other, array[i].other - i);
#endif
                                changed[i] = 1;
                            }
                    }
            }
        return shift;
    }
}
