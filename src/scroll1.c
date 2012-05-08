/* Sets in the desired `screen' the rendition of the cursor (if the X window is focused).
 * Also, sets   h->oldcursor */

/* main_screen is used to get all info, apart from the text/render buffer.
 * That is taken from screen. */
/* INTPROTO */
void
set_cursor_characters(rxvt_t *r,screen_t *main_screen, screen_t *screen,
		      rend_t* cc1, rend_t* cc2, int16_t ocrow, char *morecur)
{
    /* we'll use some aliases */
    struct rxvt_hidden *h = r->h;
    rend_t	   *srp;	/* screen-rend-pointer, drawn-rend-pointer   */

    unsigned char   setoldcursor;
    rend_t	    ccol1,	/* Cursor colour       */
		    ccol2;	/* Cursor colour2      */


    if ((main_screen->flags & Screen_VisibleCursor) && r->TermWin.focus) {
	srp = &(screen->rend[main_screen->cur.row + r->TermWin.saveLines]
			    [main_screen->cur.col]);
	*srp ^= RS_RVid;	/* XOR the Reverse Video Bit */

	/* mmc: cursor uses a special color, not simply reverse: */
#ifndef NO_CURSORCOLOR
	*cc1 = *srp & (RS_fgMask | RS_bgMask);
	if (XDEPTH > 2 && ISSET_PIXCOLOR(h, Color_cursor))
	    ccol1 = Color_cursor;
	else {
#ifdef CURSOR_COLOR_IS_RENDITION_COLOR
	    ccol1 = GET_FGCOLOR(h->rstyle);
#else
	    ccol1 = Color_fg;
#endif
	}

	if (XDEPTH > 2 && ISSET_PIXCOLOR(h, Color_cursor))
	    ccol2 = Color_cursor2;
	else {
#ifdef CURSOR_COLOR_IS_RENDITION_COLOR
	    ccol2 = GET_BGCOLOR(h->rstyle);
#else
	    ccol2 = Color_bg;
#endif
	}
	*srp = SET_FGCOLOR(*srp, ccol1);
	*srp = SET_BGCOLOR(*srp, ccol2);
#endif	/* NO_CURSORCOLOR */
#ifdef MULTICHAR_SET
	if (IS_MULTI1(*srp)) {
	    if (main_screen->cur.col < r->TermWin.ncol - 2
		&& IS_MULTI2(*++srp))
		*morecur = 1;
	} else if (IS_MULTI2(*srp)) {
	    if (main_screen->cur.col > 0 && IS_MULTI1(*--srp))
		*morecur = -1;
	}
	if (*morecur) {
	    *srp ^= RS_RVid;
# ifndef NO_CURSORCOLOR
	    *cc2 = *srp & (RS_fgMask | RS_bgMask);
	    *srp = SET_FGCOLOR(*srp, ccol1);
	    *srp = SET_BGCOLOR(*srp, ccol2);
# endif
	}
#endif	/* MULTICHAR_SET */
    }
    /* make sure no outline cursor is left around */
    setoldcursor = 0;
    if (ocrow != -1) {
	if (main_screen->cur.row + r->TermWin.view_start != ocrow
	    || main_screen->cur.col != h->oldcursor.col) {
	    if (ocrow < r->TermWin.nrow
		&& h->oldcursor.col < r->TermWin.ncol) {
		r->drawn_rend[ocrow][h->oldcursor.col] ^= (RS_RVid | RS_Uline);
#ifdef MULTICHAR_SET
		if (h->oldcursormulti) {
		    int col;
		    col = h->oldcursor.col + h->oldcursormulti;
		    if (col < r->TermWin.ncol)
			r->drawn_rend[ocrow][col] ^= (RS_RVid | RS_Uline);
		}
#endif
	    }
	    if (r->TermWin.focus
		|| !(main_screen->flags & Screen_VisibleCursor))
		h->oldcursor.row = -1;
	    else
		setoldcursor = 1;
	}
    } else if (!r->TermWin.focus)
	setoldcursor = 1;

    if (setoldcursor) {
	if (main_screen->cur.row + r->TermWin.view_start >= r->TermWin.nrow)
	    h->oldcursor.row = -1;
	else {
	    h->oldcursor.row = main_screen->cur.row + r->TermWin.view_start;
	    h->oldcursor.col = main_screen->cur.col;
#ifdef MULTICHAR_SET
	    h->oldcursormulti = *morecur;
#endif
	}
    }
}

/* INTPROTO */
void
cleanup_cursor_and_outline(rxvt_t *r, screen_t *screen, rend_t cc1, rend_t cc2,
			   unsigned char morecur)
{
    /* Added: */
    rend_t	   *srp;
    struct rxvt_hidden *h = r->h;


    if (r->TermWin.focus) {
	srp = &(screen->rend[screen->cur.row + r->TermWin.saveLines]
		[screen->cur.col]);
	*srp ^= RS_RVid;
#ifndef NO_CURSORCOLOR
	*srp = (*srp & ~(RS_fgMask | RS_bgMask)) | cc1;
#endif
#ifdef MULTICHAR_SET
	if (morecur) {
	    srp += morecur;
	    *srp ^= RS_RVid;
# ifndef NO_CURSORCOLOR
	    *srp = (*srp & ~(RS_fgMask | RS_bgMask)) | cc2;
# endif
	}
#endif
    } else if (h->oldcursor.row >= 0) {
#ifndef NO_CURSORCOLOR
	unsigned long	gcmask;	/* Graphics Context mask */

	gcmask = 0;
	XGCValues      gcvalue;
	if (XDEPTH > 2 && ISSET_PIXCOLOR(h, Color_cursor)) {
	    gcvalue.foreground = r->PixColors[Color_cursor];
	    gcmask = GCForeground;
	    XChangeGC(r->Xdisplay, r->TermWin.gc, gcmask, &gcvalue);
	    gcvalue.foreground = r->PixColors[Color_fg];
	}
#endif
#if 1				/* mmc! */
	D_SCREEN((stderr, "%s XDrawRectangle: (%d, %d -- %d, %d)\n",
		  __FUNCTION__,
		  Col2Pixel(h->oldcursor.col + morecur),
		  Row2Pixel(h->oldcursor.row),
		  (unsigned int)(Width2Pixel(1 + (morecur ? 1 : 0))
				 - 1),
		  (unsigned int)(Height2Pixel(1)
				 - r->TermWin.lineSpace - 1)
		     ));
#endif
	XDrawRectangle(r->Xdisplay, drawBuffer, r->TermWin.gc,
		       Col2Pixel(h->oldcursor.col + morecur),
		       Row2Pixel(h->oldcursor.row),
		       (unsigned int)(Width2Pixel(1 + (morecur ? 1 : 0))
				      - 1),
		       (unsigned int)(Height2Pixel(1)
				      - r->TermWin.lineSpace - 1));
#ifndef NO_CURSORCOLOR
	if (gcmask)		/* restore normal colours */
	    XChangeGC(r->Xdisplay, r->TermWin.gc, gcmask, &gcvalue);
#endif
    }
}

/* INTPROTO */
void
clear_borders(rxvt_t *r, unsigned char clearfirst, unsigned char clearlast)
{

    /* mmc! Clear the first column:  The left-most border ? */
    if (clearfirst && r->TermWin.int_bwidth) {

	/*
	 * clear the whole screen height, note that width == 0 is treated
	 * specially by XClearArea
	 */
	D_SCREEN((stderr, "XClearArea FIRST %d x %d?",
		  (unsigned int)r->TermWin.int_bwidth,
		  (unsigned int)TermWin_TotalHeight()));

	XClearArea(r->Xdisplay,	 drawBuffer, 0, 0, /* r->TermWin.vt */
		   (unsigned int)r->TermWin.int_bwidth,
		   /* mmc: What is this?       Internal border width!*/
		   (unsigned int)TermWin_TotalHeight(), False);
    }

    if (clearlast && r->TermWin.int_bwidth) {

	D_SCREEN((stderr, "XClearArea LAST %d x %d @ %d?",
		  (unsigned int)r->TermWin.int_bwidth,
		  (unsigned int)TermWin_TotalHeight(),
		  r->TermWin.width + r->TermWin.int_bwidth
		     ));
	/*
	 * clear the whole screen height, note that width == 0 is treated
	 * specially by XClearArea
	 */
	XClearArea(r->Xdisplay, drawBuffer, /* r->TermWin.vt */
		   r->TermWin.width + r->TermWin.int_bwidth, 0,
		   (unsigned int)r->TermWin.int_bwidth,
		   (unsigned int)TermWin_TotalHeight(), False);
    }
}
