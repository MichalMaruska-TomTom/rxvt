
/* \param offset is where the screen starts in the scrollback buffer.
   \param step is the scrolling factor
   \param base and \param last define the interval or rows
         (which should be scrolled by the step) */
int
verify_scroll(rxvt_t *r, int step, int base, int last, int offset)
{
    /*  +-------      +
     *  |            /| skip 2
     *  +----base---+
     *  |
     *  |
     *  |
     *  |
     *  +----------
     *  |
     *  +----------
     */
  int row, col;
    /* scroll up: previous 2 is now 1 */
    /* row? */
    for(row = base; row< r->h->prev_nrow - step - last; row++) {
      int col;
      /* col? */
      for (col = 0; col < r->screen.tlen[row + offset]; col++)
        if (r->screen.rend[row + offset][col] != r->drawn_rend[row + step][col]) {
          int diff = r->screen.rend[row + offset][col] ^ r->drawn_rend[row + step][col];

          if ((r->screen.text[row + offset][col] == r->drawn_text[row + step][col])
              && (r->drawn_text[row + step][col] == ' ')
              && !(diff &  ~RS_fgMask))
            {
              /* fprintf(stderr, "difference only in foreground, on a 'space'\n"); */
            }

          else {
#if 0
            fprintf(stderr, "difference detected at %d/%d: %.1s/%d %c/%d, (%d %d %x) in %s%s%s\n",
                    col,
                    r->screen.tlen[row + offset],
                    (char*) r->screen.text[row + offset]+col,  r->screen.text[row + offset][col],
                    r->drawn_text[row + step][col],r->drawn_text[row + step][col],

                    r->screen.rend[row + offset][col],
                    r->drawn_rend[row + step][col],
                    diff,
                    (diff & RS_fgMask)?"foreground color":"",
                    (diff & RS_bgMask)?"background color":"",
                    (diff & (RS_Bold | RS_Blink | RS_Uline | RS_acsFont | RS_ukFont | RS_RVid))
                    ? "bold etc":""
                    );
#endif
            goto wrong;
          }
        }
#if 0
      /* if (r->h->prev_nrow > 3)  */
      if (0 == memcmp((char*) (r->screen.rend[row + offset]),
                      (char*) (r->drawn_rend[row + step]),
                      r->screen.tlen[row + offset] * sizeof(rend_t)))
        fprintf(stderr, "%d & %d are identical! (%d)\n", row, row + step,
                r->screen.tlen[row + offset] * sizeof(rend_t));
      else {
        fprintf(stderr, "%d & %d are different!\n%s\n%s\n",
                row, row + step,
                r->drawn_text[row + step],
                r->screen.text[row + offset]);
        goto wrong;
      }
#endif
    }
    return 1;
  wrong:
    return 0;
}


#define SLEEP 0
/* lines  {base ... last (inclusive!)} should move by step. (in text row units)
 * case |step| > 1:
 *  step > 0  ... moving DOWN.    <0  UP
 * for step<0 .... we move also lines  {base ...(base + step)} upwards.
 */
#define DEBUG_PX 0
/* INTPROTO */
void
scroll_fluidly(rxvt_t* r, int step, int base, int last, int row_offset)
/* fixme:last is included! */
{
#if DEBUG_PX
    fprintf(stderr, "%s: %d ....%d by %d\n", __FUNCTION__, base, last, step);
    /* fixme:  base +1? */
#endif
    /* Row2Pixel Gives the line-base?
     *  --- ascent
     *
     *  --- `base' = 0
     *  --- descent ?
     *  - - - line skip
     *
     *  --- next line `base'
     */
    int start_px = Row2Pixel(base); /* + r->TermWin.font->ascent; */
    int shift_px = Row2Pixel(step + base) - Row2Pixel(base);
    unsigned int height_px = (unsigned int)Height2Pixel((last +1) - base);
    /* r->h->prev_nrow - base - last) */

    /* shift_px -=2;*/   /* r->TermWin.font->ascent */
#if DEBUG_PX
    fprintf(stderr,"block: from %d of %d pixel lines: shift by %d\n", start_px, height_px, shift_px);
#endif

    /* In my model I can use 2 determination of the microstep:
       Either I want a fixed number of steps, or I want some fixed lenght.
       */
    int substeps = abs(shift_px) / r->h->scrollstep;  /* 16 */

    /* To distribute the last step (if the total length is not divisible by the #of steps)
       uniformly I recalculate in each step, and thus  .... almost pattern.       
    int step_px = shift_px / substeps; */


    int so_far = 0; /* the sum of movement */
    /* unsigned int height = height_px;*/    /* the height of the moving block */

    /* fprintf(stderr, "doing it in %d microsteps, so each one by %d pixels\n", substeps, step_px);*/

    int j;
    for (j = 1; j <= substeps; j++) /* from 0 or 1 ? */
        {
            /* Better rounding. */
            int step_px = ((shift_px * j) / substeps) - so_far;
            
            /* The moving rectangle moves! And below/above we can render the lines! And those lines might
             * move too! */
#if DEBUG_PX 
            fprintf(stderr,"step %d: by %d pixels\n", j, step_px);
#endif
            /*                    +-------+
             *   I can move even this part!
             *   +-----------+
             *   Moved text starts
             *   here  
             *    and
             *   ends here
             *   -------------
             *
             *
             * I have 2 options: either scroll/move already rendered bottom pixels, or redraw
             * the stuff: First I will redraw!  I implemented the second one!
             */
            
            if (step > 0) {
#if DEBUG_PX
                fprintf(stderr,"our block is %d long, but we can move below it: from %d\n",
                        height_px,
                        height_px + (shift_px - so_far),
                        start_px + so_far
                        /* start_px + shift_px */
                        );
#endif 

                /* [02 nov 06] Let's move the rendered top pixels too! */
                XCopyArea(r->Xdisplay, drawBuffer, drawBuffer,
                          r->TermWin.gc,
                          /* start */
                          0, start_px,
                          /* dimension: */
                          (unsigned int)TermWin_TotalWidth(),
                          height_px + shift_px - step_px, /* fixme! */
                          /* destination */
                          0, start_px + step_px);
            } else {
#if DEBUG_PX
                fprintf(stderr,"our block is %d, but we can move above it: from %d, so %d\n",
                        start_px + so_far,
                        start_px + shift_px - step_px,
                        height_px + - (shift_px - so_far) /* fixme! - */
                        /* start_px + shift_px */
                        );
#endif 
                XCopyArea(r->Xdisplay, drawBuffer, drawBuffer,
                          r->TermWin.gc,
                          /* start */
                          0, start_px + shift_px - step_px,
                          /* dimension: */
                          (unsigned int)TermWin_TotalWidth(),
                          height_px - (shift_px - step_px),
                          0, start_px + shift_px); /* so_far + step_px */
            }

#if SLEEP
            /* Debugging: */
            {
                XFlush(r->Xdisplay);
                sleep(1);
            }
#endif


            /* fixme: I should set all `XFillRectangle' to use r->TermWin.background_gc instead
               of tricks! */
            /* fixme: I should render a line to a pixmap, and Xcopy it here! */
            if (step > 0)
                {
#if 1
                    /* fixme: I have to limit the drawing to a rectangle!, so GC */
                    XRectangle rect[1];
                    rect[0].x =0;
                    rect[0].y =0;
                    rect[0].width = (unsigned int)TermWin_TotalWidth();
                    rect[0].height = step_px; /* so_far +  */
                    XSetClipRectangles(r->Xdisplay,
                                       r->TermWin.gc, /* r->TermWin.background_gc, */
                                       /* clip_x_origin, clip_y_origin, */
                                       /* fixme:  */
                                       0, start_px, /*  + ascent!  so_far */
                                       rect, 1,
                                       Unsorted);

                    
                    {
                      /* Since we really have to redraw some scanline, we have to mark the
                         memory data as not reflected on the screen: */
                        int j;
                        for (j = base; j < base + step + 1; j++) /*  (step / so_far) */
                            {
                                rxvt_blank_line(r->drawn_text[j], r->drawn_rend[j],
                                                (unsigned int)r->TermWin.ncol,
                                                RS_needs_redraw);
                            }
                    }

                    /* fixme: For now we draw more. We might scroll some of the drawn pixels!
                       todo! */
                    /* Now draw the exposed pixel lines: */
                    {
                        unsigned char clearfirst;
                        unsigned char clearlast;
                        redraw_matrix(r, &(r->screen), row_offset,
                                             base, base + step + 1,
                                             &clearfirst, &clearlast, (so_far + step_px) - shift_px);
                    }

                    /* Undo the clipping: */
                    XSetClipMask(r->Xdisplay, r->TermWin.gc, None);
#else
                    /* we clean the top: */
                    XFillRectangle(r->Xdisplay, drawBuffer, r->TermWin.background_gc,
                                   0, start_px + so_far,
                                   (unsigned int)TermWin_TotalWidth(), step_px);
#endif
                } else {        /* step < 0     moving up!*/
#if DEBUG_PX
                fprintf(stderr,"XFillRect: %d - %d\n",height_px + start_px + so_far - step_px,
                        step_px);
#endif

#if 1
                XRectangle rect[1];
                rect[0].x =0;
                rect[0].y =0;
                rect[0].width = (unsigned int)TermWin_TotalWidth();
                rect[0].height = - step_px;


                XSetClipRectangles(r->Xdisplay,
                                   r->TermWin.gc,
                                   0, start_px + height_px + step_px, /* step_px < 0! */
                                   rect, 1,
                                   Unsorted);
                /* With some shift: so_far ?  */

                int j;
                for (j = last + step; j <= last; j++) /*  (step / so_far) */
                    {
                        rxvt_blank_line(r->drawn_text[j], r->drawn_rend[j],
                                        (unsigned int)r->TermWin.ncol,
                                        RS_needs_redraw);
                    }
                {
                    unsigned char clearfirst;
                    unsigned char clearlast;

                    /* wo move up! so we render a bit bellow */
                    redraw_matrix(r, &(r->screen), row_offset,
                                         last + step, last+1,
                                         &clearfirst, &clearlast, (so_far + step_px) - shift_px);
                }
                /* Undo: */
                XSetClipMask(r->Xdisplay, r->TermWin.gc, None);
#else
                /* Clean the bottom: */
                XFillRectangle(r->Xdisplay, drawBuffer, r->TermWin.background_gc,
                               0,
                               /*
                                 height_px - (shift_px - step_px) +
                                 start_px + shift_px - step_px
                               */
                               height_px + start_px + step_px,
                               (unsigned int)TermWin_TotalWidth(), - step_px);
#endif
            }

            so_far += step_px;
            /* height_px -= step_px; */
#if SLEEP
            /* Debugging: */
            {
                XFlush(r->Xdisplay);
                sleep(1);
            }
#endif

            /* sleep n milliseconds! */

            XFlush(r->Xdisplay);
            {
                fd_set readfds;
                fd_set writefds;

                struct timeval timeout =  {
                    0/* tv_sec */,
                    10000 /* tv_usec */
                };
                timeout.tv_usec = r->h->scrollpause;
                /* fprintf(stderr, "sleeping %d\n", timeout.tv_usec); */
                FD_ZERO(&readfds);
                FD_ZERO(&writefds);
                
                /* FD_CLR(int fd, fd_set *set);*/
                select(1, /* &readfds, &writefds */
                       NULL, NULL, NULL,
                       &timeout);
                if (timeout.tv_usec)
                    fprintf(stderr, "remains %lu\n", timeout.tv_usec);
            }
        }
#if DEBUG_PX
    fprintf(stderr,"needed %d pixels, done %d\n", shift_px, so_far);
#endif
}


extern int
heckel(int num_lines,int line_len, text_t** buffer, text_t** copy, char* changed);

#define DEBUG_smooth 0

/* \param row_offset is offset to the scrollback buffer! */
static void
try_to_scroll_smoothly(rxvt_t *r, int row_offset)
{
  unsigned char clearfirst;
  unsigned char clearlast;

  int nrows = r->TermWin.nrow;
  char *changed = (char*) alloca(sizeof(char) * nrows); /* todo: bitmap/Boolean! */
  // CHECK_CONSISTENCY(r, __FUNCTION__,0);
  /* bug: r->drawn_text is small */
  int step = heckel (nrows, r->TermWin.ncol, r->drawn_text, r->screen.text + row_offset, changed);

#if  DEBUG_smooth || 0 /* 1  ... not debugging now!*/
  fprintf(stderr, "heckel says %d\n", step);
#endif
  if (step) {
    /* xxxxxxxx   --------
     * --------   yyyyyyyy
     * yyyyyyyy
     */

    /* Find where the scrolled region starts. There's no point to disturb w/ drawing
     * something different and then scrolling it to the correct position. */
    int base = 0;
    while (changed[base] && base < nrows) /* This should be redundant!! fixme! */
      {base++;}

    int last = nrows - 1;   /* the last row which moves! (i.e. inclusive)  we can have start==last! */
    
    while (changed[last] && last > 0)
      {last --;}
    /* For symetry I would set:
     * last ++; */

#if DEBUG_smooth
    fprintf(stderr, "scrolling region %d-%d by %d (nrows = %d)\n", base, last, step, nrows);
#endif
    
    /* These should be the upper/lower bounds of changed. Once there might be more regions. */
#if 0
    verify_scroll(r, step, base, last, row_offset);
#endif



    /* Before scrolling, update (redraw) the 3 regions: */

    /*  the static lines `above' (moving region) */
    if (step > 0)           /* mmc:  moving upwards? */
      {
#if DEBUG_smooth
        fprintf(stderr, "updating lines %d ...%d\n", 0, base -1);      /* fixme:  base - step */
#endif
        redraw_matrix(r, &(r->screen), row_offset, 0, base, &clearfirst, &clearlast,0);
        /* base -1 ! */
      }    
    else
      {
#if DEBUG_smooth
        fprintf(stderr, "updating lines %d ...%d\n", 0, base + step-1);
        /* fixme:  base - step */
#endif
        redraw_matrix(r, &(r->screen), row_offset, 0, base + step,
                             &clearfirst, &clearlast,0); /* base -1 ! */
        /* fixme:  See                                           ^^^^^^^^*/
      };
        
        
        
    /* What moves: */
#if DEBUG_smooth
    fprintf(stderr, "updating what will move, lines %d ...%d\n", base, last);
    /* fixme:  base +1? */
#endif
    redraw_matrix(r, &(r->screen), row_offset + step, base, last +1,
                         &clearfirst, &clearlast,0);
        
    /* Below, static: */
    if (step > 0)
      {
#if DEBUG_smooth
        fprintf(stderr, "updating bottom lines %d ...%d\n", last + step + 1, nrows -1);
#endif
        redraw_matrix(r, &(r->screen), row_offset, last + step + 1 , nrows,
                             &clearfirst, &clearlast,0);
      }
    else
      {
#if DEBUG_smooth 
        fprintf(stderr, "updating bottom lines %d ...%d\n", last +1, nrows -1);
        /* fixme:  base +1? */
#endif
        redraw_matrix(r, &(r->screen), row_offset, last + (-step) + 1 , nrows,
                             &clearfirst, &clearlast,0);
      };
        
            
    int quantity = abs(step);
        
    /* Shift the data: we hw-move the display, so we have to shift r->rend & r->text! */
    text_t** temp_text;
    rend_t** temp_rend;
    int j;

    temp_text = alloca(quantity * sizeof(text_t*));
    temp_rend = alloca(quantity * sizeof(rend_t*));

    if (step > 0)
      for (j = 0; j < quantity; j++){
#if DEBUG_smooth
        fprintf(stderr, "%ssaving line %d%s\n", color_red, last + j +1, color_reset);
#endif
        temp_text[j] = r->drawn_text[last + j + 1];
        temp_rend[j] = r->drawn_rend[last + j + 1];

        r->drawn_text[last + j + 1] = NULL;
      }
    else 
      /* These rows are useless, but we have to keep the memory:  */
      for (j = 0; j < quantity; j++){
#if DEBUG_smooth
        fprintf(stderr, "%ssaving line %d%s\n", color_red, base - j - 1, color_reset);
#endif
        temp_text[j] = r->drawn_text[base - j -1];
        temp_rend[j] = r->drawn_rend[base - j -1];

        r->drawn_text[base - j -1] = NULL;
      };
        

    /* mmc: What is  `last'? Is that row also moved, or is it the upper bound? */
    /* Do the shift in buffers: */
    if (step > 0)
      {
#if DEBUG_smooth
        fprintf(stderr, "%soverwriting lines %d..%d%s\n",
                color_red,last + step, base + step, color_reset); /* last included? */        
#endif
        for (j = last; j >= base;  j--) /* r->h->prev_nrow - step - last */
          {
            if (! r->drawn_text[j])
              {
                fprintf(stderr, "ERROR %d!\n", j);
                exit(-1);
              }
            /* fixme:  < last ? */
            r->drawn_text[j + step] = r->drawn_text[j]; /*  ??? */
            r->drawn_rend[j + step] = r->drawn_rend[j];
            r->drawn_text[j] = NULL;
          }
      }
    else
      {
#if DEBUG_smooth 
        fprintf(stderr, "%soverwriting lines %d..%d%s\n",
                color_red, base + step, last + step,color_reset); /* last included? */ 
#endif
        for (j = base; j <= last;  j++) /* r->h->prev_nrow - step - last */
          {
            /* fixme:  < last ? */
            if (! r->drawn_text[j])
              {
                fprintf(stderr, "ERROR %d!\n", j);
                exit(-1);
              }
            r->drawn_text[j + step] = r->drawn_text[j]; /*  ??? */
            r->drawn_rend[j + step] = r->drawn_rend[j];

            /* not necessary now: */
            r->drawn_text[j] = NULL;
          }
      };
        

    /* And reuse the memory:  */
    for (j = 0; j < quantity; j++)
      {
        int index;
        if (step > 0)
          index = base + j;
        else
          /* fixme:   was last -j -1 ! */
          index = last - j; /*  [last-1   - step] is overwritten by [last-1] */
#if DEBUG_smooth
        fprintf(stderr, "%srestoring at line %d%s\n", color_red, index, color_reset);
#endif
        if (r->drawn_text[index])
          {
            fprintf(stderr, "ERROR %d is not NULL!\n", index);
            exit(-1);
          }
        /* This is to recover the allocated memory. */
        r->drawn_text[index] = temp_text[j];
        r->drawn_rend[index] = temp_rend[j];

        /* These lines should be marked as need-redraw! */
        rxvt_blank_line(r->drawn_text[index], r->drawn_rend[index],
                        (unsigned int)r->TermWin.ncol,
                        RS_needs_redraw);
      }

    /* sleep(2); */
    scroll_fluidly(r, step, base, last, row_offset);


    if (step > 0)
      {
#if DEBUG_smooth
        /* above, the static lines */
        fprintf(stderr, "updating lines %d ...%d\n", base, base + step-1);
        /* fixme:  base - step */
#endif
        redraw_matrix(r, &(r->screen), row_offset, base, base + step,
                             &clearfirst, &clearlast,0);
        /* base -1 ! */
        /* fixme:  See                                           ^^^^^^^^*/
      }
    else
      {
#if DEBUG_smooth
        fprintf(stderr, "updating lines %d ...%d\n", last + step + 1, last);
        /* fixme:  base - step */
#endif
        redraw_matrix(r, &(r->screen), row_offset, last + step +1, last +1,
                             &clearfirst, &clearlast,0); 
      };
        
    /* XFlush(r->Xdisplay); */
#if 0
    fprintf(stderr, "XCopyArea -> sleep now!\n");
    sleep(1);
#endif
  }
}

